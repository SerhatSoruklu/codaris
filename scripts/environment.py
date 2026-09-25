"""Private dev/prod configuration shared by all project launchers.
Never evaluate env files as shell code. Never load backend values for a frontend build.
"""
import os
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
FRONTEND_KEYS = {'CODARIS_ENV', 'CODARIS_PRODUCTION', 'CODARIS_SITE_URL',
                 'CODARIS_FRONTEND_PORT', 'CODARIS_API_PORT'}

def read_file(path):
    values = {}
    for number, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        key, separator, value = line.partition('=')
        if not separator or not re.fullmatch(r'[A-Z][A-Z0-9_]*', key) or key in values:
            raise ValueError(f'Invalid or duplicate environment key in {path.name}:{number}; value withheld')
        values[key] = value
    return values

def select_environment(mode, component, root=ROOT, inherited=None):
    if mode not in ('development', 'production') or component not in ('frontend', 'backend'):
        raise ValueError('Choose development or production, and frontend or backend')
    path = root / 'config' / f'{component}.{mode}.env'
    if not path.is_file():
        raise ValueError(f'Missing {path.name}. Run python scripts/init-env.py, then configure it.')
    values = read_file(path)
    if values.get('CODARIS_ENV') != mode:
        raise ValueError(f'{path.name} does not match the requested environment')
    if component == 'frontend':
        if values.keys() - FRONTEND_KEYS:
            raise ValueError(f'{path.name} contains non-public settings; use the backend file')
        if values.get('CODARIS_PRODUCTION') != ('1' if mode == 'production' else '0'):
            raise ValueError(f'{path.name} has an inconsistent production flag')
    # An existing production shell must not leak its database/SMTP into dev or
    # into the frontend compiler. The selected file is authoritative.
    parent = os.environ if inherited is None else inherited
    env = {key: value for key, value in parent.items()
           if not key.startswith(('CODARIS_', 'PG', 'SMTP_', 'MAIL_'))
           and key not in ('DATABASE_URL', 'PORT')}
    env.update(values)
    env['CODARIS_ENV_READY'] = component
    env['CODARIS_PRODUCTION'] = '1' if mode == 'production' else '0'
    return env

def validate_pair(mode, root=ROOT):
    from urllib.parse import urlsplit
    frontend=select_environment(mode,'frontend',root,inherited={})
    backend=select_environment(mode,'backend',root,inherited={})
    origin=urlsplit(backend.get('CODARIS_ORIGIN',''))
    if mode=='development':
        if (frontend.get('CODARIS_API_PORT')!=backend.get('CODARIS_PORT') or
            origin.scheme!='http' or origin.hostname!='127.0.0.1' or
            str(origin.port)!=frontend.get('CODARIS_FRONTEND_PORT')):
            raise ValueError('Development frontend ports and backend origin do not match')
    elif frontend.get('CODARIS_SITE_URL')!=backend.get('CODARIS_ORIGIN') or origin.scheme!='https':
        raise ValueError('Production frontend canonical origin and backend origin must match HTTPS')
