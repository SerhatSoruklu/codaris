#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
DB_HOST="${CODARIS_DB_LOCAL_HOST:-127.0.0.1}"
DB_PORT="${CODARIS_DB_LOCAL_PORT:-15432}"
DB_NAME="codaris"
DB_USER="codaris_migrator"

if ! command -v pg_isready >/dev/null 2>&1 || ! pg_isready -q -h "$DB_HOST" -p "$DB_PORT"; then
    echo "No CODARIS database tunnel is listening at ${DB_HOST}:${DB_PORT}. Start the database tunnel task first." >&2
    exit 1
fi

read -r -s -p "Password for ${DB_USER} (input hidden): " DB_PASSWORD
printf '\n'
if [[ -z "$DB_PASSWORD" ]]; then
    echo 'A non-empty migration password is required.' >&2
    exit 1
fi

umask 077
PASSFILE="$(mktemp "${TMPDIR:-/tmp}/codaris-pgpass.XXXXXX")"
cleanup() {
    unset DB_PASSWORD ESCAPED_PASSWORD
    rm -f -- "$PASSFILE"
}
trap cleanup EXIT HUP INT TERM

# Escape libpq password-file separators. The credential lives in a mode-600
# temporary file only while migrations and the grants are running.
ESCAPED_PASSWORD="${DB_PASSWORD//\\/\\\\}"
ESCAPED_PASSWORD="${ESCAPED_PASSWORD//:/\\:}"
printf '%s:%s:%s:%s:%s\n' "$DB_HOST" "$DB_PORT" "$DB_NAME" "$DB_USER" "$ESCAPED_PASSWORD" > "$PASSFILE"
unset DB_PASSWORD ESCAPED_PASSWORD

export PGPASSFILE="$PASSFILE" PGHOST="$DB_HOST" PGPORT="$DB_PORT" PGDATABASE="$DB_NAME" PGUSER="$DB_USER"
unset PGPASSWORD CODARIS_DATABASE_URL

if [[ "$(psql -X -Atqc "SELECT current_database() || ':' || current_user")" != "${DB_NAME}:${DB_USER}" ]]; then
    echo 'Refusing migration: connected database or role does not match the CODARIS live migration target.' >&2
    exit 1
fi
if [[ "$(psql -X -Atqc "SELECT (NOT r.rolsuper AND NOT r.rolcreatedb AND NOT r.rolcreaterole AND pg_get_userbyid(d.datdba) = current_user) FROM pg_roles r CROSS JOIN pg_database d WHERE r.rolname = current_user AND d.datname = current_database()")" != 't' ]]; then
    echo 'Refusing migration: codaris_migrator must be a non-privileged owner of the target database.' >&2
    exit 1
fi

cd "$ROOT"
python3 "$ROOT/scripts/db-migrate.py"
# Keep schema ownership with the non-runtime migration role. The API and mail
# worker share codaris_app and receive only the table operations they use.
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT USAGE ON SCHEMA app TO codaris_app;'
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT SELECT, INSERT, UPDATE ON app.users, app.accounts TO codaris_app;'
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT SELECT, INSERT, DELETE ON app.sessions, app.action_tokens TO codaris_app;'
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT SELECT, INSERT, UPDATE, DELETE ON app.mail_outbox, app.contact_outbox TO codaris_app;'
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT SELECT, INSERT, UPDATE, DELETE ON app.rate_limits TO codaris_app;'
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT SELECT, INSERT, DELETE ON app.learning_progress TO codaris_app;'
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT SELECT, INSERT, UPDATE ON app.membership_credentials TO codaris_app;'
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT USAGE, SELECT ON SEQUENCE app.users_id_seq, app.mail_outbox_id_seq, app.contact_outbox_id_seq TO codaris_app;'
if [[ "$(psql -X -Atqc "SELECT 1 FROM pg_roles WHERE rolname='codaris_readonly'")" == '1' ]]; then
    psql -X -v ON_ERROR_STOP=1 -c \
        'GRANT USAGE ON SCHEMA app TO codaris_readonly; GRANT SELECT ON app.users, app.external_profiles, app.learning_progress, app.schema_migrations TO codaris_readonly;'
    echo 'Optional codaris_readonly reporting grants refreshed.'
fi
echo 'Live CODARIS migrations applied and API runtime privileges refreshed.'
