#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

python3 "$ROOT/scripts/project.py" dev check-env
"$ROOT/scripts/db-local.sh" start
"$ROOT/scripts/db-migrate.sh" development

read -r FRONTEND_PORT API_PORT < <(python3 - "$ROOT" <<'PY'
import sys
from pathlib import Path
sys.path.insert(0, str(Path(sys.argv[1]) / 'scripts'))
from environment import select_environment
front = select_environment('development', 'frontend')
back = select_environment('development', 'backend')
print(front['CODARIS_FRONTEND_PORT'], back['CODARIS_PORT'])
PY
)
HEALTH_URL="http://127.0.0.1:${API_PORT}/api/health"
API_PID=""
if ! python3 - "$HEALTH_URL" <<'PY'
import sys
import urllib.request
try:
    with urllib.request.urlopen(sys.argv[1], timeout=2) as response:
        raise SystemExit(0 if response.status == 200 else 1)
except Exception:
    raise SystemExit(1)
PY
then
    mkdir -p "$ROOT/.local/log"
    chmod 700 "$ROOT/.local" "$ROOT/.local/log"
    API_LOG="$ROOT/.local/log/account-api.log"
    "$ROOT/scripts/run-server.sh" development >"$API_LOG" 2>&1 &
    API_PID=$!
    cleanup() {
        if [[ -n "$API_PID" ]] && kill -0 "$API_PID" 2>/dev/null; then
            kill -TERM "$API_PID" 2>/dev/null || true
            wait "$API_PID" 2>/dev/null || true
        fi
    }
    trap cleanup EXIT INT TERM
    ready=0
    for _ in {1..60}; do
        if python3 - "$HEALTH_URL" <<'PY'
import sys
import urllib.request
try:
    with urllib.request.urlopen(sys.argv[1], timeout=2) as response:
        raise SystemExit(0 if response.status == 200 else 1)
except Exception:
    raise SystemExit(1)
PY
        then
            ready=1
            break
        fi
        if ! kill -0 "$API_PID" 2>/dev/null; then
            cat "$API_LOG" >&2
            echo 'Account API exited before becoming ready.' >&2
            exit 1
        fi
        sleep 1
    done
    if [[ "$ready" != 1 ]]; then
        cat "$API_LOG" >&2
        echo 'Account API did not become ready; check its log above.' >&2
        exit 1
    fi
fi

echo "Account API healthy at ${HEALTH_URL}"
FRONTEND_URL="http://127.0.0.1:${FRONTEND_PORT}"
if python3 - "$FRONTEND_URL/api/health" <<'PY'
import sys
import urllib.request
try:
    with urllib.request.urlopen(sys.argv[1], timeout=2) as response:
        raise SystemExit(0 if response.status == 200 else 1)
except Exception:
    raise SystemExit(1)
PY
then
    echo "CODARIS is already being served at ${FRONTEND_URL}"
    "$ROOT/scripts/open-browser.sh" &
    if [[ -n "$API_PID" ]]; then wait "$API_PID"; fi
    exit 0
fi

"$ROOT/scripts/open-browser.sh" &
"$ROOT/scripts/run-client.sh" development
