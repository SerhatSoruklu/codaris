#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-${CODARIS_ENV:-development}}"
if [[ -z "${1:-}" && -z "${CODARIS_ENV:-}" && "${CODARIS_PRODUCTION:-0}" == 1 ]]; then MODE=production; fi
if [[ "${CODARIS_ENV_READY:-}" != backend ]]; then
    exec python3 "$ROOT/scripts/project.py" "$MODE" migrate
fi
export PGHOST="${PGHOST:-$ROOT/.local/postgres/socket}"
export PGPORT="${PGPORT:-55432}"
export PGDATABASE="${PGDATABASE:-codaris}"
export PGUSER="${PGUSER:-codaris_app}"
export PGCONNECT_TIMEOUT="${PGCONNECT_TIMEOUT:-5}"
exec python3 "$ROOT/scripts/db-migrate.py"
