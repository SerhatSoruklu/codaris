#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT/scripts/build-server.sh"
export PGHOST="${PGHOST:-$ROOT/.local/postgres/socket}"
export PGPORT="${PGPORT:-55432}"
export PGDATABASE="${PGDATABASE:-codaris}"
export PGUSER="${PGUSER:-codaris_app}"
export PGCONNECT_TIMEOUT="${PGCONNECT_TIMEOUT:-5}"
exec "$ROOT/build/linux/codaris_server"
