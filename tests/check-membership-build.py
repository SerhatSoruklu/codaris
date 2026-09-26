#!/usr/bin/env python3
"""Real account UI is consistent across modes; staff fixtures stay unavailable."""
import os
import re
import subprocess
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/client'
membership_options = (ROOT / 'src/shared/membership_options.h').read_text()
membership_country_rows = re.findall(r'^\s*\{"([^\"]+)", "([a-z]{2})"\},\s*$', membership_options, re.M)
demo_data = (ROOT / 'src/client/demo_data.h').read_text().split('static const size_t country_count', 1)[0]
demo_country_rows = re.findall(r'^\s*\{"([^\"]+)", "([A-Z]{2})",', demo_data, re.M)
assert len(demo_country_rows) == 23
assert len(membership_country_rows) == len(demo_country_rows) + 50
assert len({name for name, _ in membership_country_rows}) == len(membership_country_rows)
assert len({code for _, code in membership_country_rows}) == len(membership_country_rows)
assert {name for name, _ in membership_country_rows if name in {n for n, _ in demo_country_rows}} == {n for n, _ in demo_country_rows}
roles_source = membership_options.split('membership_roles[]', 1)[1].split('membership_role_count', 1)[0]
membership_roles = re.findall(r'^\s*\{"([^\"]+)", "([^\"]+)"\},\s*$', roles_source, re.M)
assert membership_roles == [
    ('Developer / Engineer', 'Developer / Engineer'),
    ('AI / ML Engineer', 'AI / ML Engineer'),
    ('Security Engineer / Researcher', 'Security Engineer / Researcher'),
    ('Researcher', 'Researcher'),
    ('Systems / Infrastructure Engineer', 'Systems / Infrastructure Engineer'),
    ('Technical Founder / Entrepreneur', 'Technical Founder / Entrepreneur'),
    ('Open-source Maintainer / Contributor', 'Open-source Maintainer / Contributor'),
    ('Product / UX Designer', 'Product / UX Designer'),
    ('Educator / Technical Writer', 'Educator / Technical Writer'),
    ('Community organiser', 'Community Organiser'),
    ('Student', 'Student'),
    ('Other', 'Other'),
]
try:
    for production in (False, True):
        env = dict(os.environ, CODARIS_PRODUCTION='1' if production else '0')
        subprocess.run([sys.executable,str(ROOT/'scripts/build-pages.py')],env=env,check=True)
        subprocess.run([sys.executable,str(ROOT/'tests/check-site.py')],env=env,check=True)
        flag_files = list((OUT / 'assets/country-flags').glob('*.svg'))
        assert {flag.stem for flag in flag_files} == {code for _, code in membership_country_rows}
        assert (OUT / 'assets/COUNTRY-FLAGS-LICENSE.txt').is_file()
        for route in ('join','login','dashboard','verify-email','reset-password'):
            page=(OUT/route/'index.html').read_text()
            assert 'src="/host.js"' in page, route
            assert 'DEVELOPMENT PREVIEW' not in page and 'sample password' not in page.lower()
        join=(OUT/'join/index.html').read_text()
        assert 'data-account-form="register"' in join and 'id="join-fields" disabled' in join
        assert 'id="join-country-trigger"' in join and 'role="listbox"' in join
        assert 'country-picker-option' not in join # C/Wasm creates safe text-only option nodes at runtime.
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
