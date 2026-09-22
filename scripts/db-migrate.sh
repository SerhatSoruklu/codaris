#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export PGHOST="${PGHOST:-$ROOT/.local/postgres/socket}"
export PGPORT="${PGPORT:-55432}"
export PGDATABASE="${PGDATABASE:-codaris}"
export PGUSER="${PGUSER:-codaris_app}"
export PGCONNECT_TIMEOUT="${PGCONNECT_TIMEOUT:-5}"
# The baseline has one migration; preserve it unchanged and skip if applied.
if [[ "$(psql -X -At -v ON_ERROR_STOP=1 -c "SELECT to_regclass('app.schema_migrations') IS NOT NULL")" == t ]] &&
   [[ "$(psql -X -At -v ON_ERROR_STOP=1 -c "SELECT count(*) FROM app.schema_migrations WHERE version = '001_initial'")" == 1 ]]; then
    printf '001_initial already applied.\n'
else
    psql -X -v ON_ERROR_STOP=1 -f "$ROOT/db/migrations/001_initial.sql"
fi
