#!/usr/bin/env python3
"""Assemble static HTML routes; no runtime framework or third-party packages."""
import html
import json
import os
import shutil
import re
import runpy
from pathlib import Path
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[1]
runpy.run_path(str(ROOT / 'scripts/build-language-docs.py'))
runpy.run_path(str(ROOT / 'scripts/build-framework-docs.py'))
runpy.run_path(str(ROOT / 'scripts/build-database-docs.py'))
runpy.run_path(str(ROOT / 'scripts/build-topic-library.py'))
WEB = ROOT / 'web'
OUT = ROOT / 'build/client'
site = json.loads((WEB / 'site.json').read_text(encoding='utf-8'))
origin = os.environ.get('CODARIS_SITE_URL', site['origin']).rstrip('/')
production = os.environ.get('CODARIS_PRODUCTION') == '1'
if origin and production:
    parsed = urlsplit(origin)
    if not re.fullmatch(r'https://[A-Za-z0-9.-]+(?::[0-9]{1,5})?', origin) or parsed.scheme != 'https' or not parsed.hostname or parsed.path or parsed.query or parsed.fragment or parsed.username or parsed.password:
        raise SystemExit('CODARIS_SITE_URL must be an HTTPS origin, e.g. https://your-domain.example')
# Raster assets are copied here so Bash and PowerShell share the same policy.
(OUT / 'assets').mkdir(parents=True, exist_ok=True)
for asset in (WEB / 'assets').iterdir():
    if asset.suffix not in {'.png', '.webp'}:
        continue
    shutil.copy2(asset, OUT / 'assets' / asset.name)
runpy.run_path(str(ROOT / 'scripts/build-lucide-sprite.py'))
icon_out = OUT / 'assets/icons'
icon_out.mkdir(parents=True, exist_ok=True)
shutil.copy2(WEB / 'assets/icons/LICENSE', icon_out / 'LICENSE')

pages = json.loads((WEB / 'pages.json').read_text(encoding='utf-8'))
shell = (WEB / 'index.html').read_text(encoding='utf-8')
header = (WEB / 'partials/header.html').read_text(encoding='utf-8')
footer = (WEB / 'partials/footer.html').read_text(encoding='utf-8')
headers = json.loads((ROOT / 'deploy/security-headers.json').read_text(encoding='utf-8'))
# frame-ancestors is an HTTP-only directive; production Nginx enforces it.
meta_csp = headers['Content-Security-Policy'].replace("; frame-ancestors 'none'", '')
urls = []
seen_routes = set()
for page in pages:
    slug = page['slug']
    content_name = page.get('file', slug)
    route_pattern = r'[a-z0-9]+(?:-[a-z0-9]+)*(?:/[a-z0-9]+(?:-[a-z0-9]+)*)*'
    if not re.fullmatch(r'[a-z0-9]+(?:-[a-z0-9]+)*', content_name) or (slug and not re.fullmatch(route_pattern, slug)) or slug in seen_routes:
        raise SystemExit('Invalid or duplicate page route')
    seen_routes.add(slug)
    route = '/' + slug + '/' if slug else '/'
    # Apply the brand suffix once for every route and metadata surface.
    seo_title = page['title'] + ' | CODARIS'
    title = html.escape(seo_title, quote=True)
    description = html.escape(page['description'], quote=True)
    if page.get('indexing') not in {'index', 'noindex'}:
        raise SystemExit('Every route must declare indexing as index or noindex: ' + route)
    indexable = page['indexing'] == 'index'
    seo = '<meta name="robots" content="noindex, follow">' if not indexable or not production or not origin else ''
    if origin:
        url = origin + route
        seo += '\n<link rel="canonical" href="' + html.escape(url, quote=True) + '">'
        seo += '\n<meta property="og:url" content="' + html.escape(url, quote=True) + '">'
        if indexable and production:
            urls.append(url)
            data = {'@context': 'https://schema.org', '@graph': [
                {'@type': 'WebPage', '@id': url + '#webpage', 'name': seo_title, 'description': page['description'], 'url': url, 'isPartOf': {'@id': origin + '/#website'}, 'about': {'@id': origin + '/#organization'}}]}
            if not slug:
                data['@graph'][:0] = [
                    {'@type': 'WebSite', '@id': origin + '/#website', 'name': 'CODARIS', 'url': origin + '/', 'publisher': {'@id': origin + '/#organization'}},
                    {'@type': 'Organization', '@id': origin + '/#organization', 'name': 'CODARIS', 'alternateName': 'Coalition Of Developers Advancing Responsible Intelligent Systems', 'url': origin + '/', 'sameAs': ['https://x.com/codarisorg']}]
            if slug:
                crumb_names = slug.split('/')
                crumb_items = [{'@type': 'ListItem', 'position': 1, 'name': 'Home', 'item': origin + '/'}]
                for position, name in enumerate(crumb_names, start=2):
                    crumb_path = '/' + '/'.join(crumb_names[:position - 1]) + '/'
                    crumb_page = next((item for item in pages if item['slug'] == '/'.join(crumb_names[:position - 1])), None)
                    crumb_items.append({'@type': 'ListItem', 'position': position, 'name': crumb_page['label'] if crumb_page else page['label'], 'item': origin + crumb_path})
                data['@graph'].append({'@type': 'BreadcrumbList', 'itemListElement': crumb_items})
            seo += '\n<script type="application/ld+json">' + json.dumps(data).replace('<', '\\u003c') + '</script>'
    if origin:
        image_url = origin + '/assets/Codaris_Flag.png'
        alt = 'CODARIS flag with its member community emblem.'
        seo += '\n<meta property="og:image" content="' + html.escape(image_url, quote=True) + '">'
        seo += '\n<meta property="og:image:type" content="image/png">'
        seo += '\n<meta property="og:image:width" content="1448">'
        seo += '\n<meta property="og:image:height" content="1086">'
        seo += '\n<meta property="og:image:alt" content="' + html.escape(alt, quote=True) + '">'
        seo += '\n<meta name="twitter:image" content="' + html.escape(image_url, quote=True) + '">'
        seo += '\n<meta name="twitter:image:alt" content="' + html.escape(alt, quote=True) + '">'
    runtime = ''
    legal_css = '<link rel="stylesheet" href="/legal.css">' if slug in {'privacy', 'terms'} else ''
    meaning_css = '<link rel="stylesheet" href="/meaning.css">' if slug in {'', 'mission'} else ''
    contact_css = '<link rel="stylesheet" href="/contact.css">' if slug == 'contact' else ''
    if page.get('interactive'):
        runtime = '''<p id="runtime-status" role="status">Loading interactive features…</p>
<noscript><p class="runtime-error">Interactive features require JavaScript and WebAssembly. You can still read the pages and navigate the site.</p></noscript>
<script src="/host.js" defer></script>
<script id="codaris-runtime" src="/codaris.js" defer></script>'''
    breadcrumb = '<nav class="wrap breadcrumb" aria-label="Breadcrumb"><a href="/">Home</a><span aria-hidden="true">/</span><span aria-current="page">' + html.escape(page['label']) + '</span></nav>' if slug else ''
    content = (WEB / 'pages' / (page.get('file', slug) + '.html')).read_text(encoding='utf-8')
    content = content.replace('{{LEARNING_LIBRARY}}', (WEB / 'partials/learning-library.html').read_text(encoding='utf-8'))
    content = re.sub(r'<!-- DEVELOPMENT START -->(.*?)<!-- DEVELOPMENT END -->',
                     lambda match: '' if production else match.group(1), content, flags=re.S)
    if page.get('development_only'):
        content = '<section class="wrap section resource-page"><p class="eyebrow">MEMBERSHIP / COMING SOON</p><h1 class="page-title">' + title + '</h1><p>Staff access is not available yet.</p><a class="button" href="/join/">Explore membership</a></section>'
    values = {'CSP': html.escape(meta_csp, quote=True), 'TITLE': title, 'DESCRIPTION': description, 'SEO': seo, 'LEGALCSS': legal_css, 'MEANINGCSS': meaning_css, 'CONTACTCSS': contact_css, 'HEADER': header.replace('href="' + route + '"', 'href="' + route + '" aria-current="page"'), 'BREADCRUMB': breadcrumb, 'CONTENT': content, 'DEVELOPMENT': 'false' if production else 'true', 'FOOTER': footer, 'RUNTIME': runtime}
    document = shell
    for key, value in values.items():
        document = document.replace('{{' + key + '}}', value)
    target = OUT / slug
    target.mkdir(parents=True, exist_ok=True)
    (target / 'index.html').write_text(document, encoding='utf-8')
robots = 'User-agent: *\nAllow: /\nDisallow: /api/\n'
if origin and production:
    (OUT / 'sitemap.xml').write_text('<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">' + ''.join('<url><loc>' + html.escape(url) + '</loc></url>' for url in urls) + '</urlset>\n', encoding='utf-8')
    robots += 'Sitemap: ' + origin + '/sitemap.xml\n'
else:
    (OUT / 'sitemap.xml').unlink(missing_ok=True)
(OUT / 'robots.txt').write_text(robots, encoding='utf-8')
shutil.copy2(WEB / 'legal.css', OUT / 'legal.css')
shutil.copy2(WEB / 'meaning.css', OUT / 'meaning.css')
shutil.copy2(WEB / 'contact.css', OUT / 'contact.css')
shutil.copy2(WEB / 'auth-nav.js', OUT / 'auth-nav.js')
print(f'Built {len(pages)} static routes; ' + ('production SEO enabled.' if production and origin else 'preview noindex; set CODARIS_PRODUCTION=1 for production indexing.'))

# Generate the Nginx include outside the public document root.
deploy_out = ROOT / 'build/deploy'
deploy_out.mkdir(parents=True, exist_ok=True)
(deploy_out / 'security-headers.conf').write_text(''.join(
    'add_header ' + name + ' "' + value + '" always;\n' for name, value in headers.items()), encoding='utf-8')
# A real 404 document prevents static hosts from falling back to the homepage.
not_found = shell
values.update({'TITLE': 'Page Not Found | CODARIS', 'DESCRIPTION': 'This page could not be found.', 'SEO': '<meta name="robots" content="noindex, follow">', 'LEGALCSS': '', 'MEANINGCSS': '', 'CONTACTCSS': '', 'HEADER': header, 'BREADCRUMB': '', 'RUNTIME': '', 'CONTENT': '<section class="wrap section resource-page"><h1 class="page-title">Page not found.</h1><p>The page may have moved. <a href="/">Return to the homepage.</a></p></section>'})
for key, value in values.items():
    not_found = not_found.replace('{{' + key + '}}', value)
(OUT / '404.html').write_text(not_found, encoding='utf-8')

# The same builder is invoked by Bash and PowerShell.
runpy.run_path(str(ROOT / "scripts/build-admin.py"), run_name="__main__")
