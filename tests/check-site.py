#!/usr/bin/env python3
"""Validate the deployable site without third-party test dependencies."""
import json
import re
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit
from xml.etree import ElementTree

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/client'
REGISTRY = json.loads((ROOT / 'web/pages.json').read_text())

class Document(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.ids = set()
        self.links = []
        self.assets = []
        self.headings = 0
        self.csp = None
        self.noindex = False
        self.script = None
        self.script_text = ''

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        assert not any(key.lower().startswith('on') for key in attrs), 'Inline event handler'
        assert 'style' not in attrs, 'Inline style attribute'
        for key in ('href', 'src', 'action'):
            if key in attrs:
                assert not re.match(r'\s*(javascript|data|vbscript):', attrs[key], re.I), 'Unsafe URL'
        if 'id' in attrs:
            assert attrs['id'] not in self.ids, 'Duplicate element ID'
            self.ids.add(attrs['id'])
        if tag == 'h1':
            self.headings += 1
        if tag == 'a' and 'href' in attrs:
            self.links.append(attrs['href'])
        if tag in ('img', 'script') and attrs.get('src', '').startswith('/'):
            self.assets.append(attrs['src'])
        if tag == 'link' and attrs.get('href', '').startswith('/'):
            self.assets.append(attrs['href'])
        if tag == 'meta' and attrs.get('http-equiv', '').lower() == 'content-security-policy':
            self.csp = attrs['content']
        if tag == 'meta' and attrs.get('name') == 'robots':
            self.noindex = 'noindex' in attrs['content']
        if tag == 'script':
            self.script = attrs
            self.script_text = ''
            if 'src' in attrs:
                assert attrs['src'] in ('/host.js', '/codaris.js'), 'Unreviewed executable script'
            else:
                assert attrs.get('type') == 'application/ld+json', 'Inline executable script'

    def handle_data(self, data):
        if self.script is not None:
            self.script_text += data

    def handle_endtag(self, tag):
        if tag == 'script' and self.script is not None:
            if 'src' not in self.script:
                json.loads(self.script_text)
            self.script = None

pages = {}
for path in OUT.rglob('*.html'):
    doc = Document()
    doc.feed(path.read_text(encoding='utf-8'))
    assert doc.headings == 1, (path, 'Expected one H1')
    assert doc.csp and "default-src 'none'" in doc.csp, path
    assert "'unsafe-inline'" not in doc.csp and "'unsafe-eval'" not in doc.csp, path
    assert "'wasm-unsafe-eval'" in doc.csp and "form-action 'none'" in doc.csp, path
    assert '{{' not in path.read_text(encoding='utf-8'), 'Unexpanded template'
    pages[path] = doc
for path, doc in pages.items():
    for asset in doc.assets:
        assert (OUT / asset.lstrip('/')).is_file(), (path, asset)
    for link in doc.links:
        parsed = urlsplit(link)
        if parsed.scheme or parsed.netloc:
            assert parsed.scheme == 'https', link
            continue
        target = OUT / unquote(parsed.path).lstrip('/') / 'index.html' if parsed.path else path
        assert target in pages, (path, link)
        if parsed.fragment:
            assert parsed.fragment in pages[target].ids, (path, link)
for entry in REGISTRY:
    doc = pages[OUT / entry['slug'] / 'index.html']
    if entry.get('placeholder'):
        assert doc.noindex, entry['slug']
expected = sum(not entry.get('placeholder') for entry in REGISTRY)
urls = list(ElementTree.parse(OUT / 'sitemap.xml').iter('{http://www.sitemaps.org/schemas/sitemap/0.9}loc'))
assert len(urls) == expected
host = (ROOT / 'web/host.js').read_text()
assert not re.search(r'innerHTML|outerHTML|document\.write|\beval\s*\(|new\s+Function', host)
assert 'onerror=' not in (ROOT / 'scripts/build-pages.py').read_text()
for script in ('build-client.sh', 'build-client.ps1'):
    assert '-sDYNAMIC_EXECUTION=0' in (ROOT / 'scripts' / script).read_text()
headers = json.loads((ROOT / 'deploy/security-headers.json').read_text())
assert "frame-ancestors 'none'" in headers['Content-Security-Policy']
assert headers['X-Content-Type-Options'] == 'nosniff'
print(f'PASS: {len(pages)} documents; CSP, scripts, IDs, links, assets, metadata and build hardening.')
