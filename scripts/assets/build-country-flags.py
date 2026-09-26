#!/usr/bin/env python3
"""Build the local country flag symbol sprite from pinned flag-icons SVGs."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import urllib.request
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
VERSION = 'v7.5.0'
SOURCE = f'https://raw.githubusercontent.com/lipis/flag-icons/{VERSION}/flags/4x3/{{code}}.svg'
HEADER = ROOT / 'src/shared/membership_options.h'
OUT_DIR = ROOT / 'web/assets/country-flags'
OLD_SPRITE = ROOT / 'web/assets/country-flags.svg'
LICENSE = ROOT / 'web/assets/COUNTRY-FLAGS-LICENSE.txt'

codes = re.findall(r'^\s*\{"[^\"]+", "([a-z]{2})"\},\s*$', HEADER.read_text(), re.M)
if not codes or len(codes) != len(set(codes)):
    raise SystemExit('Country code list is empty or contains duplicates')

def fetch(code):
    request = urllib.request.Request(SOURCE.format(code=code), headers={'User-Agent': 'CODARIS-country-flag-bundler'})
    with urllib.request.urlopen(request, timeout=20) as response:
        data = response.read().decode('utf-8')
    root = ET.fromstring(data)
    if root.tag.rsplit('}', 1)[-1] != 'svg' or root.attrib.get('viewBox') != '0 0 640 480':
        raise SystemExit(f'Unexpected upstream SVG for {code}')
    return code, data

OUT_DIR.mkdir(parents=True, exist_ok=True)
OLD_SPRITE.unlink(missing_ok=True)
with ThreadPoolExecutor(max_workers=8) as pool:
    for code, body in pool.map(fetch, codes):
        (OUT_DIR / f'{code}.svg').write_text(body)
license_url = f'https://raw.githubusercontent.com/lipis/flag-icons/{VERSION}/LICENSE'
request = urllib.request.Request(license_url, headers={'User-Agent': 'CODARIS-country-flag-bundler'})
with urllib.request.urlopen(request, timeout=20) as response:
    LICENSE.write_bytes(response.read())
print(f'Built {len(codes)} flag SVGs from flag-icons {VERSION}: {OUT_DIR.relative_to(ROOT)}')
