#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT/scripts/build-client.sh"
printf 'CODARIS browser client: http://localhost:%s (Ctrl+C to stop)\n' "${PORT:-8080}"
exec python3 -m http.server "${PORT:-8080}" --bind 127.0.0.1 --directory "$ROOT/build/client"
