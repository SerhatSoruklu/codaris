#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
URL="$(python3 - "$ROOT" <<'PYENV'
import sys
from pathlib import Path
sys.path.insert(0,str(Path(sys.argv[1])/'scripts'))
from environment import select_environment
env=select_environment('development','frontend')
port=int(env['CODARIS_FRONTEND_PORT'])
if not 1024 <= port <= 65535:raise SystemExit('Invalid development frontend port')
print(f'http://127.0.0.1:{port}')
PYENV
)"
# Wait for the local build/server before opening the desktop browser.
python3 - "$URL" <<'PY'
import sys, time, urllib.request
for _ in range(120):
    try:
        with urllib.request.urlopen(sys.argv[1], timeout=1) as response:
            if response.status == 200:
                break
    except OSError:
        pass
    time.sleep(1)
else:
    raise SystemExit('CODARIS did not start within 120 seconds. Check the run terminal.')
PY
if command -v powershell.exe >/dev/null; then
    powershell.exe -NoProfile -Command "Start-Process '$URL'"
else
    exec xdg-open "$URL"
fi
