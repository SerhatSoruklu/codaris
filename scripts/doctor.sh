#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
missing=0
for tool in cc cmake emcc python3 pg_config psql; do
    if command -v "$tool" >/dev/null; then
        printf '[OK] %s: %s\n' "$tool" "$(command -v "$tool")"
    else
        printf '[MISSING] %s\n' "$tool"
        missing=1
    fi
done
exit "$missing"
