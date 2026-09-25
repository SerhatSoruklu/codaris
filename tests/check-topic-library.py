#!/usr/bin/env python3
"""Verify all supplied research survives rendering and all topics are reachable."""
import hashlib
import html
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'data/topics'
load = lambda path: json.loads(path.read_text())
manifest = load(DATA / 'manifest.json')
checksums = {}
for line in (DATA / 'SHA256SUMS').read_text().splitlines():
    digest, path = line.split('  ', 1)
    checksums[path] = digest
editorial = load(DATA / 'editorial.json')
foundations = load(DATA / 'foundations.json')
assert len(manifest) == 13 and sum(m['count'] for m in manifest) == 2673
assert len(foundations) == 6
subprocess.run([sys.executable, str(ROOT / 'scripts/build-topic-library.py')], check=True)


def leaves(value):
    if isinstance(value, dict):
        for item in value.values():
            yield from leaves(item)
    elif isinstance(value, list):
        for item in value:
            yield from leaves(item)
    elif value is not None and value != '':
        yield str(value)


outputs = []
for pack in manifest:
    directory = DATA / pack['slug']
    pack_checksums = {path.removeprefix(pack['slug'] + '/'): digest
                      for path, digest in checksums.items()
                      if path.startswith(pack['slug'] + '/')}
    assert set(pack_checksums) == set(pack['source_files'])
    for name, checksum in pack_checksums.items():
        assert hashlib.sha256((directory / name).read_bytes()).hexdigest() == checksum, name
    data = load(directory / pack['catalog'])
    rows = data.get('runtimes', data.get('catalogue'))
    path = ROOT / 'web/pages' / (pack['slug'] + '.html')
    outputs.append(path)
    raw = path.read_text()
    decoded = html.unescape(raw)
    assert len(rows) == pack['count'] == raw.count('data-catalogue-name=')
    assert len(editorial[pack['slug']]['guides']) == raw.count('class="language-guide"') == 6
    for row in rows:
        assert f'id="entry-{row["catalog_id"]}"' in raw
    for value in leaves(data):
        assert value in decoded, (pack['slug'], value)
    for value in leaves(load(directory / 'workbook-tables.json')):
        assert value in decoded, (pack['slug'], 'workbook', value)
    for readme in directory.glob('README*.md'):
        assert readme.read_text() in decoded
    assert 'not live measurements' in decoded
    assert 'No per-tool gender percentages are inferred' in decoded

for topic in foundations:
    path = ROOT / 'web/pages' / (topic['slug'] + '.html')
    outputs.append(path)
    raw = path.read_text()
    assert raw.count('class="language-guide"') == 6
    assert raw.count('<code>') == 6
    for guide in topic['guides']:
        for value in guide.values():
            assert value in html.unescape(raw), (topic['slug'], value)
    assert topic['project'] in html.unescape(raw)

library = load(DATA / 'library.json')
assert len(library) == len({t['slug'] for t in library}) == 22
assert {t['topic_id'] for t in library} == set(range(22))
assert sum(t.get('count', 0) for t in library) == 4621
partial = ROOT / 'web/partials/learning-library.html'
hub = ROOT / 'web/pages/topics.html'
outputs.extend([partial, hub, DATA / 'library.json'])
for topic in library:
    assert f'data-topic="{topic["topic_id"]}"' in partial.read_text()
    for page in (partial, hub):
        assert f'href="/{topic["slug"]}/" aria-label="View topic:' in page.read_text()
assert partial.read_text().count('class="button topic-open"') == 22
assert hub.read_text().count('data-topic=') == 22
assert '#define MEMBER_TOPIC_COUNT 22' in (ROOT / 'src/client/membership.h').read_text()
before = {path: path.read_bytes() for path in outputs}
subprocess.run([sys.executable, str(ROOT / 'scripts/build-topic-library.py')], check=True)
assert all(path.read_bytes() == original for path, original in before.items())
print('PASS: 13 intact source packs, 2,673 complete entries, all workbook cells, six full guides, 22 topic links, deterministic output')
