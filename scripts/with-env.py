#!/usr/bin/env python3
"""Load a private KEY=value file without shell evaluation, then run a command.
Works on Linux/WSL and Windows. Existing environment is the default; file wins.
"""
import os
import re
import subprocess
import sys
from pathlib import Path
if len(sys.argv) < 3:
    raise SystemExit('Usage: python scripts/with-env.py PRIVATE_ENV COMMAND [ARGS...]')
env = dict(os.environ)
for line in Path(sys.argv[1]).read_text(encoding='utf-8').splitlines():
    line = line.strip()
    if not line or line.startswith('#'):
        continue
    key, separator, value = line.partition('=')
    if not separator or not re.fullmatch(r'[A-Z][A-Z0-9_]*', key):
        raise SystemExit('Invalid environment line (contents withheld)')
    env[key] = value
raise SystemExit(subprocess.call(sys.argv[2:], env=env))
