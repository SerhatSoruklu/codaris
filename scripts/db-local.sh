#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# Private development cluster: Unix socket only, no TCP listener.
PG_BIN="$(pg_config --bindir)"
BASE="$ROOT/.local/postgres"
export PGHOST="$BASE/socket" PGPORT=55432 PGDATABASE=codaris PGUSER=codaris_app
case "${1:-start}" in
start)
    mkdir -p "$BASE/socket"
    chmod 700 "$ROOT/.local" "$BASE" "$BASE/socket"
    if [[ ! -f "$BASE/data/PG_VERSION" ]]; then
        "$PG_BIN/initdb" -D "$BASE/data" -U codaris_app --auth-local=trust --auth-host=reject >/dev/null
    fi
    if ! "$PG_BIN/pg_ctl" -D "$BASE/data" status >/dev/null 2>&1; then
        "$PG_BIN/pg_ctl" -D "$BASE/data" -l "$BASE/server.log" -o "-k '$BASE/socket' -p 55432 -c listen_addresses=''" -w start
    fi
    if [[ "$(psql -d postgres -Atc "SELECT 1 FROM pg_database WHERE datname = 'codaris'")" != 1 ]]; then
        createdb codaris
    fi
    printf 'Local CODARIS database ready (private Unix socket).\n'
    ;;
stop)
    if [[ -f "$BASE/data/PG_VERSION" ]] && "$PG_BIN/pg_ctl" -D "$BASE/data" status >/dev/null 2>&1; then
        "$PG_BIN/pg_ctl" -D "$BASE/data" -m fast -w stop
    fi
    ;;
*) printf 'Usage: %s [start|stop]\n' "$0" >&2; exit 2 ;;
esac
