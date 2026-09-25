#!/usr/bin/env python3
"""Apply sequential migrations using libpq environment and psql's ON_ERROR_STOP."""
import os
import subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def sql(statement):
    return subprocess.check_output(['psql', '-X', '-At', '-v', 'ON_ERROR_STOP=1', '-c', statement], text=True).strip()
for path in sorted((ROOT / 'db/migrations').glob('[0-9][0-9][0-9]_*.sql')):
    if sql("SELECT to_regclass('app.schema_migrations') IS NOT NULL") == 't':
        applied = sql('SELECT version FROM app.schema_migrations').splitlines()
        if path.stem in applied:
            print(path.stem + ' already applied.')
            continue
    subprocess.run(['psql', '-X', '-v', 'ON_ERROR_STOP=1', '-f', str(path)], check=True)
