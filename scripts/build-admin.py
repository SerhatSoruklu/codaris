#!/usr/bin/env python3
"""Build a separate static staff-preview site; never enables production auth."""
import os
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/admin'
SOURCE = ROOT / 'build/client'
production = os.environ.get('CODARIS_PRODUCTION') == '1'
MAIN_ORIGIN = 'https://codaris.org'
ADMIN_ORIGIN = 'https://admin.codaris.org'
OUT.mkdir(parents=True, exist_ok=True)

def link(match):
    path = match.group(1)
    if path.startswith('/staff-login/'):
        return 'href="' + path.replace('/staff-login/', '/login/', 1) + '"'
    if path.startswith('/staff-dashboard/'):
        return 'href="' + path.replace('/staff-dashboard/', '/dashboard/', 1) + '"'
    return 'href="' + MAIN_ORIGIN + path + '"'

routes = {'': 'staff-login', 'login': 'staff-login', 'dashboard': 'staff-dashboard'}
for target, slug in routes.items():
    document = (SOURCE / slug / 'index.html').read_text(encoding='utf-8')
    document = re.sub(r'href="(/[^"\s]*)"', link, document)
    # Static asset URLs remain same-origin on the admin site.
    document = document.replace('href="https://codaris.org/styles.css"', 'href="/styles.css"')
    document = document.replace('href="https://codaris.org/assets/emblem.svg"', 'href="/assets/emblem.svg"')
    document = re.sub(r'<link rel="canonical" href="[^"]+">', '<link rel="canonical" href="' + ADMIN_ORIGIN + '/' + (target + '/' if target else '') + '">', document)
    document = re.sub(r'<meta property="og:url" content="[^"]+">', '<meta property="og:url" content="' + ADMIN_ORIGIN + '/' + (target + '/' if target else '') + '">', document)
    document = document.replace('noindex, follow', 'noindex, nofollow')
    document = document.replace('<body ', '<body data-admin-site="true" ')
    document = re.sub(r'<header class="site-header wrap">.*?</header>', '<header class="site-header wrap"><a class="brand" href="/login/"><img src="/assets/emblem.svg" width="34" height="34" alt="">CODARIS / STAFF</a><a class="text-link" href="https://codaris.org/dashboard/">Member dashboard ↗</a></header>', document, flags=re.S)
    directory = OUT / target
    directory.mkdir(parents=True, exist_ok=True)
    (directory / 'index.html').write_text(document, encoding='utf-8')
for asset in ('styles.css', 'host.js'):
    shutil.copyfile(ROOT / 'web' / asset, OUT / asset)
for asset in ('codaris.js', 'codaris.wasm'):
    if (SOURCE / asset).is_file():
        shutil.copyfile(SOURCE / asset, OUT / asset)
(OUT / 'assets').mkdir(exist_ok=True)
for asset in ('emblem.svg', 'x.svg'):
    shutil.copyfile(ROOT / 'web/assets' / asset, OUT / 'assets' / asset)
(OUT / 'robots.txt').write_text('User-agent: *\nDisallow: /\n', encoding='utf-8')
# Use a static unavailable page for unknown paths; never a dashboard fallback.
not_found = re.sub(r'href="(/[^"\s]*)"', link, (SOURCE / '404.html').read_text(encoding='utf-8'))
not_found = not_found.replace('href="https://codaris.org/styles.css"', 'href="/styles.css"').replace('href="https://codaris.org/assets/emblem.svg"', 'href="/assets/emblem.svg"')
(OUT / '404.html').write_text(not_found, encoding='utf-8')
print('Built separate admin site: build/admin (' + 'staff access unavailable' + ')')
