#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-${CODARIS_ENV:-development}}"
if [[ -z "${1:-}" && -z "${CODARIS_ENV:-}" && "${CODARIS_PRODUCTION:-0}" == 1 ]]; then MODE=production; fi
if [[ "${CODARIS_ENV_READY:-}" != frontend ]]; then
    exec python3 "$ROOT/scripts/project.py" "$MODE" build-client
fi
mkdir -p "$ROOT/build/client"
if ! command -v emcc >/dev/null && [[ -f "${EMSDK:-$HOME/emsdk}/emsdk_env.sh" ]]; then
    source "${EMSDK:-$HOME/emsdk}/emsdk_env.sh"
fi
# Ubuntu's packaged SDK uses a read-only cache. Keep generated artifacts local.
if [[ "$(command -v emcc)" == /usr/bin/emcc && -z "${EM_CONFIG:-}" ]]; then
    export EM_CACHE="$ROOT/.local/emscripten-cache"
    export EM_FROZEN_CACHE=""
fi
emcc "$ROOT/src/client/main.c" -std=c17 -Wall -Wextra -Wpedantic -O2 -sDYNAMIC_EXECUTION=0 -sSTACK_OVERFLOW_CHECK=2 -sNO_EXIT_RUNTIME=1 -sEXPORTED_RUNTIME_METHODS=ccall -sASSERTIONS=0 -sENVIRONMENT=web -o "$ROOT/build/client/codaris.js"
python3 "$ROOT/scripts/build-pages.py"
cp "$ROOT/web/styles.css" "$ROOT/web/host.js" "$ROOT/build/client/"
mkdir -p "$ROOT/build/client/assets"
cp "$ROOT/web/assets/"*.svg "$ROOT/build/client/assets/"
mkdir -p "$ROOT/build/client/assets/country-flags"
cp "$ROOT/web/assets/country-flags/"*.svg "$ROOT/build/client/assets/country-flags/"
cp "$ROOT/web/assets/COUNTRY-FLAGS-LICENSE.txt" "$ROOT/build/client/assets/"
