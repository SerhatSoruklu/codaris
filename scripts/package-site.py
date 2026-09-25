#!/usr/bin/env python3
"""Allowlisted, reproducible-content deployment bundle. No source or secrets."""
import hashlib
import re
import shutil
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
admin_site = len(sys.argv) == 3 and sys.argv[2] == '--admin'
release = sys.argv[1] if len(sys.argv) == 2 or admin_site else ''
if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,79}', release):
    raise SystemExit('Supply a safe release identifier')
source = root / ('build/admin' if admin_site else 'build/client')
target = root / 'build/package' / release
if target.exists():
    raise SystemExit('Release already packaged; choose a new identifier')
files = []
allowed_root = {'index.html', '404.html', 'styles.css', 'host.js', 'codaris.js', 'codaris.wasm', 'robots.txt', 'sitemap.xml', 'version.txt'}
for file in sorted(source.rglob('*')):
    if file.is_symlink():
        raise SystemExit('Symlinks are not allowed in site output')
    if not file.is_file():
        continue
    if file.suffix == ".html" and 'data-development="true"' in file.read_text(encoding="utf-8"):
        raise SystemExit("Refusing to package a development build. Run python scripts/project.py prod build-client.")
    relative = file.relative_to(source)
    permitted = (len(relative.parts) == 1 and relative.name in allowed_root) or (
        len(relative.parts) == 2 and relative.name == 'index.html' and re.fullmatch(r'[a-z0-9-]+', relative.parts[0])) or (
        len(relative.parts) == 2 and relative.parts[0] == 'assets' and file.suffix in {'.svg', '.png', '.webp'})
    if not permitted:
        raise SystemExit('Unexpected build artifact: ' + str(relative))
    files.append((file, relative))
for file, relative in files:
    output = target / 'site' / relative
    output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(file, output)
manifest = ''.join(hashlib.sha256(file.read_bytes()).hexdigest() + '  site/' + relative.as_posix() + '\n' for file, relative in files)
(target / 'SHA256SUMS').write_text(manifest, encoding='utf-8')
print(f'Packaged {len(files)} public files into {target}')
