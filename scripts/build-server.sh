#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-${CODARIS_ENV:-development}}"
if [[ -z "${1:-}" && -z "${CODARIS_ENV:-}" && "${CODARIS_PRODUCTION:-0}" == 1 ]]; then MODE=production; fi
if [[ "${CODARIS_ENV_READY:-}" != backend ]]; then
    exec python3 "$ROOT/scripts/project.py" "$MODE" build-server
fi
BUILD_TYPE=Debug
if [[ "$CODARIS_ENV" == production ]]; then BUILD_TYPE=Release; fi
BUILD_DIR="$ROOT/build/linux/$CODARIS_ENV"
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "$BUILD_DIR" --parallel
