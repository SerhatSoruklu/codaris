#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-${CODARIS_ENV:-development}}"
if [[ -z "${1:-}" && -z "${CODARIS_ENV:-}" && "${CODARIS_PRODUCTION:-0}" == 1 ]]; then MODE=production; fi
if [[ "${CODARIS_ENV_READY:-}" != frontend ]]; then
    exec python3 "$ROOT/scripts/project.py" "$MODE" run-client
fi
"$ROOT/scripts/build-client.sh"
exec python3 "$ROOT/scripts/serve-dev.py" --port "${CODARIS_FRONTEND_PORT:-8081}" --api-port "${CODARIS_API_PORT:-8080}"
