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
MAIL_PID=""
api_is_current() {
python3 - "$HEALTH_URL" <<'PY'
import json
import sys
import urllib.request
try:
    with urllib.request.urlopen(sys.argv[1], timeout=2) as response:
        data = json.loads(response.read())
        raise SystemExit(0 if response.status == 200 and data.get('contact_api') == 1 and data.get('credential_api') == 2 and data.get('mail_sender_aligned') is True else 1)
except Exception:
    raise SystemExit(1)
PY
}
mail_is_configured() {
python3 - "$ROOT" <<'PY'
import sys
from pathlib import Path
from urllib.parse import urlsplit
sys.path.insert(0, str(Path(sys.argv[1]) / 'scripts'))
from environment import select_environment
values = select_environment('development', 'backend')
url = urlsplit(values.get('CODARIS_SMTP_URL', ''))
authenticated = bool(values.get('CODARIS_SMTP_USER') and values.get('CODARIS_SMTP_PASSWORD'))
local_capture = url.hostname in ('127.0.0.1', 'localhost') and not values.get('CODARIS_SMTP_USER') and not values.get('CODARIS_SMTP_PASSWORD')
raise SystemExit(0 if authenticated or local_capture else 1)
PY
}
api_binary_is_stale() {
    local running_pid running_exe
    running_pid="$(ss -ltnp "sport = :${API_PORT}" 2>/dev/null | sed -n 's/.*pid=\([0-9][0-9]*\).*/\1/p' | head -n 1)"
    [[ -n "$running_pid" ]] || return 1
    running_exe="$(readlink "/proc/${running_pid}/exe" 2>/dev/null || true)"
    [[ "$running_exe" == "$ROOT/build/linux/development/codaris_server (deleted)"* ]]
}
cleanup() {
    for child_pid in "$MAIL_PID" "$API_PID"; do
        if [[ -n "$child_pid" ]] && kill -0 "$child_pid" 2>/dev/null; then
            kill -TERM "$child_pid" 2>/dev/null || true
        fi
    done
    for child_pid in "$MAIL_PID" "$API_PID"; do
        if [[ -n "$child_pid" ]]; then wait "$child_pid" 2>/dev/null || true; fi
    done
}
trap cleanup EXIT INT TERM
if ! api_is_current || api_binary_is_stale; then
    # Rebuilds replace the executable on disk, but an already-running API keeps
    # serving its old code. Restart only this project's managed development API.
    OLD_PID="$(ss -ltnp "sport = :${API_PORT}" 2>/dev/null | sed -n 's/.*pid=\([0-9][0-9]*\).*/\1/p' | head -n 1)"
    if [[ -n "$OLD_PID" ]]; then
        OLD_EXE="$(readlink "/proc/${OLD_PID}/exe" 2>/dev/null || true)"
        case "$OLD_EXE" in
            "$ROOT/build/linux/development/codaris_server"*)
                echo 'Refreshing the development account API to load the current build.'
                kill -TERM "$OLD_PID"
                for _ in {1..30}; do
                    if ! kill -0 "$OLD_PID" 2>/dev/null; then break; fi
                    sleep 0.1
                done
                ;;
            *)
                if [[ -n "$OLD_EXE" ]]; then
                    echo "A healthy but outdated API is running outside this project's development build: ${OLD_EXE}" >&2
                    echo "Stop that API process, then run ./scripts/dev.sh again." >&2
                    exit 1
                fi
                ;;
        esac
    fi
    mkdir -p "$ROOT/.local/log"
    chmod 700 "$ROOT/.local" "$ROOT/.local/log"
    API_LOG="$ROOT/.local/log/account-api.log"
    "$ROOT/scripts/run-server.sh" development >"$API_LOG" 2>&1 &
    API_PID=$!
    ready=0
    for _ in {1..60}; do
        if api_is_current; then
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
if mail_is_configured; then
    mkdir -p "$ROOT/.local/log"
    chmod 700 "$ROOT/.local" "$ROOT/.local/log"
    MAIL_LOG="$ROOT/.local/log/mail-worker.log"
    touch "$MAIL_LOG"
    chmod 600 "$MAIL_LOG"
    (
        umask 077
        while true; do
            if ! python3 "$ROOT/scripts/project.py" development mail; then
                echo 'Development mail drain failed; pending messages will be retried.' >&2
            fi
            sleep 60
        done
    ) >>"$MAIL_LOG" 2>&1 &
    MAIL_PID=$!
    echo "Development mail worker running; log: ${MAIL_LOG}"
else
    echo 'Development mail worker is paused; configure backend.development.env SMTP credentials.'
fi
FRONTEND_URL="http://127.0.0.1:${FRONTEND_PORT}"
if python3 - "$FRONTEND_URL/api/health" <<'PY'
import json
import sys
import urllib.request
try:
    with urllib.request.urlopen(sys.argv[1], timeout=2) as response:
        data = json.loads(response.read())
        raise SystemExit(0 if response.status == 200 and data.get('contact_api') == 1 and data.get('credential_api') == 2 else 1)
except Exception:
    raise SystemExit(1)
PY
then
    echo "CODARIS is already being served at ${FRONTEND_URL}"
    "$ROOT/scripts/open-browser.sh" &
    if [[ -n "$API_PID" ]]; then wait "$API_PID"
    elif [[ -n "$MAIL_PID" ]]; then wait "$MAIL_PID"
    else echo 'Development servers are already running.'
    fi
    exit 0
fi

"$ROOT/scripts/open-browser.sh" &
"$ROOT/scripts/run-client.sh" development
