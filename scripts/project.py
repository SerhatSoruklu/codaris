#!/usr/bin/env python3
"""One explicit environment selector for Bash and PowerShell workflows."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
from environment import ROOT, select_environment, validate_pair

parser = argparse.ArgumentParser()
parser.add_argument('mode', choices=('dev', 'prod', 'development', 'production'))
parser.add_argument('action', choices=('build', 'build-client', 'build-server', 'run-client',
                                     'run-server', 'mail', 'migrate', 'check-env'))
args = parser.parse_args()
mode = {'dev':'development', 'prod':'production'}.get(args.mode, args.mode)

def run_script(name, component):
    env = select_environment(mode, component)
    suffix = '.ps1' if os.name == 'nt' else '.sh'
    shell = ['powershell', '-NoProfile', '-File'] if os.name == 'nt' else ['bash']
    return subprocess.call(shell + [str(ROOT / 'scripts' / (name + suffix))], env=env)

try:
    if args.action == 'check-env':
        validate_pair(mode)
        print(f'{mode}: matching frontend and backend files selected; values withheld')
        result = 0
    elif args.action == 'build':
        result = run_script('build-client', 'frontend')
        if result == 0:
            result = run_script('build-server', 'backend')
    elif args.action == 'migrate':
        result = subprocess.call([sys.executable,str(ROOT/'scripts/db-migrate.py')],
                                 env=select_environment(mode,'backend'))
    elif args.action == 'mail':
        base = ROOT/'build'/('native' if os.name=='nt' else 'linux')/mode
        candidates = [base/('Release' if mode=='production' else 'Debug')/'codaris_server.exe',
                      base/'codaris_server.exe'] if os.name=='nt' else [base/'codaris_server']
        binary = next((path for path in candidates if path.is_file()), None)
        if binary is None:
            raise ValueError('Build the selected backend first')
        result = subprocess.call([str(binary),'--mail-once'],env=select_environment(mode,'backend'))
    elif args.action == 'run-client' and mode == 'production':
        raise ValueError('Production frontend is served by Nginx. Use prod build-client and the release runbook; the dev proxy is not a production server.')
    else:
        if args.action in ('run-server', 'run-client'):
            validate_pair(mode)
        result = run_script(args.action, 'frontend' if args.action.endswith('client') else 'backend')
except (OSError, ValueError) as error:
    # Configuration errors carry filenames/key names only, never secret values.
    print(str(error), file=sys.stderr)
    result = 2
raise SystemExit(result)
