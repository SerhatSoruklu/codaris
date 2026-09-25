#!/usr/bin/env python3
"""Real account UI is consistent across modes; staff fixtures stay unavailable."""
import os
import subprocess
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/client'
try:
    for production in (False, True):
        env = dict(os.environ, CODARIS_PRODUCTION='1' if production else '0')
        subprocess.run([sys.executable,str(ROOT/'scripts/build-pages.py')],env=env,check=True)
        subprocess.run([sys.executable,str(ROOT/'tests/check-site.py')],env=env,check=True)
        for route in ('join','login','dashboard','verify-email','reset-password'):
            page=(OUT/route/'index.html').read_text()
            assert 'src="/host.js"' in page, route
            assert 'DEVELOPMENT PREVIEW' not in page and 'sample password' not in page.lower()
        join=(OUT/'join/index.html').read_text()
        assert 'data-account-form="register"' in join and 'id="join-fields" disabled' in join
        assert 'inert' not in join and 'join-overlay' not in join
        assert 'name="password"' in join
        dashboard=(OUT/'dashboard/index.html').read_text()
        assert 'id="account-content" hidden' in dashboard
        for name in ('country','role','linkedin','github','website'):
            assert f'id="profile-{name}"' in dashboard
        assert 'id="application-reason"' in dashboard and 'name="motivation"' not in dashboard
        for route in ('staff-login','staff-dashboard'):
            page=(OUT/route/'index.html').read_text()
            assert '<form' not in page and 'src="/host.js"' not in page
        for route in ('login','dashboard'):
            page=(ROOT/'build/admin'/route/'index.html').read_text()
            assert '<form' not in page and 'noindex, nofollow' in page
        assert not (OUT/'templates').exists()
        assert not list(OUT.rglob('*.env'))
        assert 'ILLUSTRATIVE DATA' not in (OUT/'index.html').read_text()
    print('PASS: real membership UI, private account shell and unavailable staff tools in both build modes')
finally:
    subprocess.run([sys.executable,str(ROOT/'scripts/build-pages.py')],check=True)
