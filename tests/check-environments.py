#!/usr/bin/env python3
"""Prevent configuration crossover and frontend secret exposure."""
import importlib.util
import subprocess
import sys
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('environment',ROOT/'scripts/environment.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory);(root/'config').mkdir()
    for mode,production in [('development','0'),('production','1')]:
        (root/'config'/f'frontend.{mode}.env').write_text(f'CODARIS_ENV={mode}\nCODARIS_PRODUCTION={production}\nCODARIS_SITE_URL=https://codaris.org\n')
        (root/'config'/f'backend.{mode}.env').write_text(f'CODARIS_ENV={mode}\nPGDATABASE={mode}_database\nCODARIS_SMTP_PASSWORD={mode}_private_value\n')
    inherited={'PATH':'/safe/path','PGDATABASE':'production_database','PGPASSWORD':'inherited_password',
               'CODARIS_SMTP_PASSWORD':'inherited_smtp','CODARIS_ENV':'production',
               'CODARIS_PRODUCTION':'1','DATABASE_URL':'inherited_connection','SMTP_PASS':'inherited'}
    frontend=module.select_environment('development','frontend',root,inherited)
    assert frontend['CODARIS_ENV']=='development' and frontend['CODARIS_PRODUCTION']=='0'
    assert all(key not in frontend for key in ('PGDATABASE','PGPASSWORD','CODARIS_SMTP_PASSWORD','DATABASE_URL','SMTP_PASS'))
    backend=module.select_environment('development','backend',root,inherited)
    assert backend['PGDATABASE']=='development_database'
    assert backend['CODARIS_SMTP_PASSWORD']=='development_private_value' and 'PGPASSWORD' not in backend
    production=module.select_environment('production','frontend',root,inherited)
    assert production['CODARIS_PRODUCTION']=='1' and 'PGPASSWORD' not in production
    path=root/'config/frontend.development.env'
    path.write_text(path.read_text()+'CODARIS_SMTP_PASSWORD=do_not_print_me\n')
    try:module.select_environment('development','frontend',root,inherited)
    except ValueError as error:assert 'do_not_print_me' not in str(error)
    else:raise AssertionError('Frontend accepted private settings')
    path.write_text('CODARIS_ENV=production\nCODARIS_PRODUCTION=1\n')
    try:module.select_environment('development','frontend',root,inherited)
    except ValueError:pass
    else:raise AssertionError('Accepted mismatched environment')
for mode in ('development','production'):
    module.validate_pair(mode)
    for component in ('frontend','backend'):
        path=Path('config')/f'{component}.{mode}.env'
        subprocess.run(['git','check-ignore','--quiet',str(path)],cwd=ROOT,check=True)
        assert not subprocess.check_output(['git','ls-files','--',str(path)],cwd=ROOT)
assert not list((ROOT/'config').glob('*.example'))
print('PASS: dev/prod selection, inheritance isolation, frontend secret rejection, matching origins/ports and ignored real env files')
