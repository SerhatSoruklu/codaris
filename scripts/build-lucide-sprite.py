#!/usr/bin/env python3
"""Build CODARIS's small local SVG sprite from pinned Lucide source icons."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'web/assets/icons/lucide-1.48.0'
OUTPUT = ROOT / 'build/client/assets/icons/lucide.svg'

ICONS = (
    'activity', 'arrow-down', 'arrow-right', 'book-open', 'bot', 'clock-3',
    'boxes', 'building', 'cloud', 'container', 'cpu', 'database', 'download',
    'external-link', 'flask-conical', 'git-branch', 'git-fork',
    'code', 'hand-coins', 'key-round', 'layout-dashboard',
    'log-in', 'log-out', 'mail', 'mail-check', 'message-square', 'microscope',
    'monitor-cog', 'network', 'package', 'panels-top-left',
    'plug', 'radio', 'rotate-cw', 'save', 'scan-line', 'send', 'server-cog',
    'map-pin',
    'shield-check', 'user-round', 'users', 'workflow',
)

ET.register_namespace('', 'http://www.w3.org/2000/svg')
SVG = '{http://www.w3.org/2000/svg}'
symbols = ET.Element(SVG + 'svg')
for name in ICONS:
    tree = ET.parse(SOURCE / f'{name}.svg')
    source = tree.getroot()
    symbol = ET.SubElement(symbols, SVG + 'symbol', {
        'id': name,
        'viewBox': source.attrib.get('viewBox', '0 0 24 24'),
        'fill': 'none',
        'stroke': 'currentColor',
        'stroke-width': '2',
        'stroke-linecap': 'round',
        'stroke-linejoin': 'round',
    })
    for child in source:
        symbol.append(child)

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
ET.ElementTree(symbols).write(OUTPUT, encoding='utf-8', xml_declaration=True)
print(f'Built local Lucide sprite: {len(ICONS)} icons, v1.48.0.')
