#!/usr/bin/env bash
set -euo pipefail
URL="http://localhost:${PORT:-8080}"
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
