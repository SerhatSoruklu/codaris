#!/usr/bin/env python3
"""Create real private configuration once; never overwrite existing files."""
from pathlib import Path
import os
import secrets
ROOT = Path(__file__).resolve().parents[1]
config = ROOT / 'config'
config.mkdir(exist_ok=True)
for mode in ('development', 'production'):
    prod = mode == 'production'
    frontend = (f'CODARIS_ENV={mode}\nCODARIS_PRODUCTION={int(prod)}\n'
                'CODARIS_SITE_URL=https://codaris.org\n'
                'CODARIS_FRONTEND_PORT=8081\nCODARIS_API_PORT=8080\n')
    pg_host = '127.0.0.1' if prod or os.name == 'nt' else str(ROOT / '.local/postgres/socket')
    pg_port = 5432 if prod or os.name == 'nt' else 55432
    backend = (f'CODARIS_ENV={mode}\nCODARIS_PORT=8080\n'
               f'CODARIS_ORIGIN={"https://codaris.org" if prod else "http://127.0.0.1:8081"}\n'
               f'PGHOST={pg_host}\nPGPORT={pg_port}\nPGDATABASE=codaris\nPGUSER=codaris_app\nPGCONNECT_TIMEOUT=5\n'
               f'CODARIS_SMTP_URL={"smtp://smtp.gmail.com:587" if prod else "smtp://127.0.0.1:1025"}\n'
               f'CODARIS_SMTP_USER={"admin@coupyn.com" if prod else ""}\nCODARIS_SMTP_PASSWORD=\n'
               'CODARIS_MAIL_FROM=no-reply@codaris.org\n'
               f'CODARIS_MAIL_KEY={secrets.token_hex(32)}\n')
    if prod:
        backend += 'PGPASSFILE=/etc/codaris/pgpass\n'
    for component, contents in [('frontend', frontend), ('backend', backend)]:
        target = config / f'{component}.{mode}.env'
        try:
            descriptor = os.open(target, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        except FileExistsError:
            print(f'Preserved {target.name}')
            continue
        with os.fdopen(descriptor, 'w', encoding='utf-8') as output:
            output.write(contents)
        print(f'Created {target.name}')
print('Environment values are private and were not printed. Configure production database and SMTP before running production.')
