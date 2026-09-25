#!/usr/bin/env python3
"""Check full framework source coverage and deterministic documentation output."""
import html
import json
import subprocess
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'data/web-frameworks'
data=json.loads((DATA/'codaris_web_frameworks_catalog.json').read_text())
workbook=json.loads((DATA/'workbook-tables.json').read_text())
subprocess.run([sys.executable,str(ROOT/'scripts/build-framework-docs.py')],check=True)
page_path=ROOT/'web/pages/web-frameworks.html'
page=page_path.read_text()
e=lambda v:html.escape(str(v),quote=True)
assert len(data['frameworks'])==631 and page.count('data-catalogue-name=')==631
for row in data['frameworks']:
    assert f'id="framework-{row["catalog_id"]}"' in page
    assert e(row['name']) in page
    search=' | '.join(row[k] for k in ('name','category','layer','kind','primary_language','ecosystem'))
    assert f'data-catalogue-name="{e(search)}"' in page
    assert len(search.encode())<512
    for key,value in row.items():
        if value is not None and key not in ('catalog_id','initial'):
            assert e(value) in page
for name,rows in workbook.items():
    for row in rows:
        for value in row:
            if value:assert e(value) in page,(name,value)
for source in data['sources']:
    assert e(source['url']) in page
assert page.count('class="language-guide"')==12
assert 'Not reported' in page and 'not live counters' in page
subprocess.run([sys.executable,str(ROOT/'scripts/build-framework-docs.py')],check=True)
assert page_path.read_text()==page
print('PASS: all 631 entries, workbook cells, source links and deterministic output')
