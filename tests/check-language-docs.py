#!/usr/bin/env python3
"""Protect source completeness and escaped, deterministic documentation output."""
import html
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'data/programming-languages'
data = json.loads((DATA / 'codaris_programming_languages_catalog.json').read_text())
subprocess.run([sys.executable, str(ROOT / 'scripts/build-language-docs.py')], check=True)
page_path = ROOT / 'web/pages/programming-languages.html'
page = page_path.read_text()
assert len(data['languages']) == 674
assert page.count('data-catalogue-name=') == 674
for row in data['languages']:
    assert f'id="language-{row["catalog_id"]}"' in page
    assert f'data-catalogue-name="{html.escape(row["language"], quote=True)}"' in page
    assert len(row['language'].encode()) < 512, 'C DOM text buffer must hold full name'
assert page.count('class="language-guide"') == 15
# Every row of each separate research table is preserved, including zero values.
for key in ('global_github_top10_2026_q1', 'stackoverflow_used_2025', 'tiobe_2026_09', 'stackoverflow_country_respondents_2025', 'country_language_examples'):
    for row in data[key]:
        rendered = '<tr>' + ''.join('<td>' + html.escape(str(value), quote=True) + '</td>' for value in row.values()) + '</tr>'
        assert rendered in page, (key, row)
for source in data['sources']:
    assert html.escape(source['url'], quote=True) in page
assert 'Not reported' in page and 'no inferred percentages' in page
subprocess.run([sys.executable, str(ROOT / 'scripts/build-language-docs.py')], check=True)
assert page_path.read_text() == page, 'Rendering must be deterministic'
print('PASS: complete catalogue, research tables, source links and deterministic rendering')
