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

cd "$ROOT"
python3 "$ROOT/scripts/db-migrate.py"
psql -X -v ON_ERROR_STOP=1 -c \
    'GRANT SELECT ON app.users, app.external_profiles, app.learning_progress, app.schema_migrations TO codaris_readonly;'
echo 'Live CODARIS migrations applied; DBeaver read access granted to selected non-credential tables.'
