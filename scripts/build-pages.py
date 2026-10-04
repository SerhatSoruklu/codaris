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
shutil.copy2(WEB / 'assets/codaris-favicon-v2.svg', OUT / 'assets/codaris-favicon-v2.svg')
runpy.run_path(str(ROOT / 'scripts/build-lucide-sprite.py'))
icon_out = OUT / 'assets/icons'
icon_out.mkdir(parents=True, exist_ok=True)
shutil.copy2(WEB / 'assets/icons/LICENSE', icon_out / 'LICENSE')

pages = json.loads((WEB / 'pages.json').read_text(encoding='utf-8'))
shell = (WEB / 'index.html').read_text(encoding='utf-8')
header = (WEB / 'partials/header.html').read_text(encoding='utf-8')
footer = (WEB / 'partials/footer.html').read_text(encoding='utf-8')
analytics = (WEB / 'partials/analytics.html').read_text(encoding='utf-8')
if production and origin == 'https://codaris.org':
    footer = footer.replace('href="/privacy/#cookies"', 'href="/privacy/#analytics-settings"')
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
    # Keep one brand mention and enforce concise metadata on every route.
    seo_title = page['title'].strip() + ' | CODARIS'
    if seo_title.count('CODARIS') != 1 or len(seo_title) > 60:
        raise SystemExit('SEO title must contain CODARIS once and fit 60 characters: ' + route)
    if not 140 <= len(page['description']) <= 160:
        raise SystemExit('SEO description must be 140–160 characters: ' + route)
    title = html.escape(seo_title, quote=True)
    description = html.escape(page['description'], quote=True)
    if page.get('indexing') not in {'index', 'noindex'}:
        raise SystemExit('Every route must declare indexing as index or noindex: ' + route)
    if page.get('access', 'public') not in {'public', 'member'}:
        raise SystemExit('Route access must be public or member: ' + route)
    member_only = page.get('access') == 'member'
    client_guard = page.get('client_guard', False)
    if client_guard and not member_only:
        raise SystemExit('Client-guarded routes must also be member-only: ' + route)
    indexable = page['indexing'] == 'index' and not member_only
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
                    {'@type': 'Organization', '@id': origin + '/#organization', 'name': 'CODARIS', 'alternateName': 'Coalition Of Developers Advancing Responsible Intelligent Systems', 'url': origin + '/', 'sameAs': ['https://x.com/codarisorg', 'https://www.linkedin.com/company/codarisorg/']}]
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
        image_url = origin + '/assets/codaris-social-v2.png'
        alt = 'CODARIS coalition mark and BUILD. VERIFY. ADVANCE. message on a dark teal background.'
        seo += '\n<meta property="og:image" content="' + html.escape(image_url, quote=True) + '">'
        seo += '\n<meta property="og:image:secure_url" content="' + html.escape(image_url, quote=True) + '">'
        seo += '\n<meta property="og:image:type" content="image/png">'
        seo += '\n<meta property="og:image:width" content="1200">'
        seo += '\n<meta property="og:image:height" content="630">'
        seo += '\n<meta property="og:image:alt" content="' + html.escape(alt, quote=True) + '">'
        seo += '\n<meta name="twitter:image" content="' + html.escape(image_url, quote=True) + '">'
        seo += '\n<meta name="twitter:image:alt" content="' + html.escape(alt, quote=True) + '">'
    runtime = ''
    legal_css = '<link rel="stylesheet" href="/legal.css">' if slug in {'privacy', 'terms'} else ''
    meaning_css = '<link rel="stylesheet" href="/meaning.css">' if slug in {'', 'mission'} else ''
    contact_css = '<link rel="stylesheet" href="/contact.css">' if slug == 'contact' else ''
    vision_css = '<link rel="stylesheet" href="/vision.css">' if slug == 'vision' else ''
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
    home_intro = '<script src="/home-intro.js?v=2"></script>' if not slug else ''
    values = {'CSP': html.escape(meta_csp, quote=True), 'TITLE': title, 'DESCRIPTION': description, 'SEO': seo, 'HOMEINTRO': home_intro, 'LEGALCSS': legal_css, 'MEANINGCSS': meaning_css, 'CONTACTCSS': contact_css, 'VISIONCSS': vision_css, 'HEADER': header.replace('href="' + route + '"', 'href="' + route + '" aria-current="page"'), 'BREADCRUMB': breadcrumb, 'CONTENT': content, 'DEVELOPMENT': 'false' if production else 'true', 'MEMBER_ROUTE': 'true' if member_only else 'false', 'FOOTER': footer, 'RUNTIME': runtime}
    document = shell
    # Public information only: exclude forms, credentials, accounts and errors.
    values['ANALYTICS'] = analytics if production and origin == 'https://codaris.org' and indexable and slug not in {'join', 'contact'} else ''
    for key, value in values.items():
        document = document.replace('{{' + key + '}}', value)
    target = OUT / slug
    target.mkdir(parents=True, exist_ok=True)
    (target / 'index.html').write_text(document, encoding='utf-8')
member_routes = sorted(page['slug'] for page in pages if page.get('access') == 'member')
server_guarded_routes = sorted(page['slug'] for page in pages if page.get('access') == 'member' and not page.get('client_guard'))
robots = 'User-agent: *\nAllow: /\nDisallow: /api/\n' + ''.join('Disallow: /' + route + '/\n' for route in member_routes)
if origin and production:
    (OUT / 'sitemap.xml').write_text('<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">' + ''.join('<url><loc>' + html.escape(url) + '</loc></url>' for url in urls) + '</urlset>\n', encoding='utf-8')
    robots += 'Sitemap: ' + origin + '/sitemap.xml\n'
else:
    (OUT / 'sitemap.xml').unlink(missing_ok=True)
(OUT / 'robots.txt').write_text(robots, encoding='utf-8')
shutil.copy2(WEB / 'google12ca11a422470d51.html', OUT / 'google12ca11a422470d51.html')
shutil.copy2(WEB / 'legal.css', OUT / 'legal.css')
shutil.copy2(WEB / 'meaning.css', OUT / 'meaning.css')
shutil.copy2(WEB / 'contact.css', OUT / 'contact.css')
shutil.copy2(WEB / 'vision.css', OUT / 'vision.css')
api_status = (WEB / 'api-status.html').read_text(encoding='utf-8')
api_status = api_status.replace('{{CSP}}', html.escape(meta_csp, quote=True))
(OUT / 'api-status.html').write_text(api_status, encoding='utf-8')
shutil.copy2(WEB / 'api-status.css', OUT / 'api-status.css')
shutil.copy2(WEB / 'api-status.js', OUT / 'api-status.js')
shutil.copy2(WEB / 'not-found-scene.js', OUT / 'not-found-scene.js')
shutil.copy2(WEB / 'three.module.js', OUT / 'three.module.js')
shutil.copy2(WEB / 'three.core.js', OUT / 'three.core.js')
shutil.copy2(WEB / 'THREE-LICENSE.txt', OUT / 'THREE-LICENSE.txt')
shutil.copy2(WEB / 'auth-nav.js', OUT / 'auth-nav.js')
shutil.copy2(WEB / 'api-origin.js', OUT / 'api-origin.js')
shutil.copy2(WEB / 'site.webmanifest', OUT / 'site.webmanifest')
shutil.copy2(WEB / 'auth-redirect.js', OUT / 'auth-redirect.js')
shutil.copy2(WEB / 'member-access.js', OUT / 'member-access.js')
shutil.copy2(WEB / 'home-intro.js', OUT / 'home-intro.js')
print(f'Built {len(pages)} static routes; ' + ('production SEO enabled.' if production and origin else 'preview noindex; set CODARIS_PRODUCTION=1 for production indexing.'))

# Generate the Nginx include outside the public document root.
deploy_out = ROOT / 'build/deploy'
deploy_out.mkdir(parents=True, exist_ok=True)
(deploy_out / 'security-headers.conf').write_text(''.join(
    'add_header ' + name + ' "' + value + '" always;\n' for name, value in headers.items()), encoding='utf-8')
member_route_pattern = '|'.join(server_guarded_routes)
member_routes_conf = (
    'location ~ ^/(?:' + member_route_pattern + ')(?:/|$) {\n'
    '    if ($request_method !~ ^(GET|HEAD)$) { return 405; }\n'
    '    auth_request /_codaris_page_access;\n'
    '    error_page 401 =401 /member-sign-in-required/;\n'
    '    error_page 403 = /member-access-unavailable/;\n'
    '    error_page 500 502 503 504 /member-service-unavailable/;\n'
    '    try_files $uri $uri/ =404;\n'
    '}\n'
)
(deploy_out / 'member-routes.conf').write_text(member_routes_conf, encoding='utf-8')
# A real 404 document prevents static hosts from falling back to the homepage.
not_found = shell
not_found_content = r'''<section class="not-found wrap" aria-labelledby="not-found-title">
  <div class="not-found__copy">
    <p class="not-found__eyebrow"><span aria-hidden="true"></span> CODARIS / ROUTE RECOVERY</p>
    <p class="not-found__code">ERROR 404 <span>·</span> PATH NOT FOUND</p>
    <h1 id="not-found-title">This page isn’t in the system.</h1>
    <p class="not-found__message">The route is missing, moved, or still waiting to be built. Our engineer has been staring at the trace for a while.</p>
    <div class="not-found__actions">
      <a class="button" href="/">Return to CODARIS <span aria-hidden="true">↗</span></a>
      <a class="not-found__secondary" href="/mission/">Explore the mission</a>
    </div>
    <p class="not-found__trace"><span>REQUEST</span> &nbsp; RESOURCE ABSENT<br><span>STATUS</span> &nbsp; HTTP 404</p>
  </div>
  <div class="not-found__scene" data-route-scene tabindex="0" role="group" aria-labelledby="route-scene-label" aria-describedby="route-scene-help">
    <canvas class="not-found__canvas" data-route-scene-canvas aria-hidden="true"></canvas>
    <div class="not-found__scene-chrome">
      <div class="not-found__scene-label" id="route-scene-label">NODE 04 <span>·</span> NO RESPONSE</div>
      <div class="not-found__scene-tip">HOVER / TAP / ARROW KEYS TO INSPECT</div>
    </div>
    <p class="not-found__scene-fallback" data-route-scene-fallback>NODE 04 / 404 · INTERACTIVE SCENE UNAVAILABLE</p>
    <div class="not-found__scene-status" aria-hidden="true"><span>ENGINEER STATUS</span><strong>TRACE REVIEW IN PROGRESS</strong></div>
    <p class="not-found__scene-selection" data-route-scene-selection aria-live="polite">SCENE READY · SELECT A COMPONENT</p>
    <span id="route-scene-help" class="sr-only">Three-dimensional route recovery workspace. Move the pointer over or tap the developer, chair, keyboard, mouse, field device, coffee mug, room, or equipment to inspect it. Focus this scene and use the arrow keys to cycle through all components.</span>
    <script type="module" src="/not-found-scene.js"></script>
  </div>
</section>'''
values.update({'TITLE': 'Page Not Found | CODARIS', 'DESCRIPTION': 'This page could not be found.', 'SEO': '<meta name="robots" content="noindex, follow">', 'LEGALCSS': '', 'MEANINGCSS': '', 'CONTACTCSS': '', 'HEADER': header, 'BREADCRUMB': '', 'MEMBER_ROUTE': 'false', 'RUNTIME': '', 'ANALYTICS': '', 'CONTENT': not_found_content})
for key, value in values.items():
    not_found = not_found.replace('{{' + key + '}}', value)
(OUT / '404.html').write_text(not_found, encoding='utf-8')

# The same builder is invoked by Bash and PowerShell.
runpy.run_path(str(ROOT / "scripts/build-admin.py"), run_name="__main__")
