#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-${CODARIS_ENV:-development}}"
if [[ -z "${1:-}" && -z "${CODARIS_ENV:-}" && "${CODARIS_PRODUCTION:-0}" == 1 ]]; then MODE=production; fi
if [[ "${CODARIS_ENV_READY:-}" != backend ]]; then
    exec python3 "$ROOT/scripts/project.py" "$MODE" run-server
fi
"$ROOT/scripts/build-server.sh"
exec "$ROOT/build/linux/$CODARIS_ENV/codaris_server"
