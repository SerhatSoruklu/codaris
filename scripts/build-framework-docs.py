#!/usr/bin/env python3
"""Render all supplied web-technology research without runtime dependencies."""
import html
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'data/web-frameworks'
data = json.loads((DATA / 'codaris_web_frameworks_catalog.json').read_text(encoding='utf-8'))
workbook = json.loads((DATA / 'workbook-tables.json').read_text(encoding='utf-8'))
guides = json.loads((DATA / 'guides.json').read_text(encoding='utf-8'))
e = lambda value: html.escape(str(value), quote=True)
def link(url, label):
    if not url.startswith('https://'):
        raise ValueError('Expected HTTPS source')
    return f'<a href="{e(url)}" target="_blank" rel="noopener noreferrer">{e(label)} ↗</a>'
def table(rows, caption):
    width = len(rows[0])
    result = f'<div class="research-table" tabindex="0" role="region" aria-label="{e(caption)}"><table><caption>{e(caption)}</caption><thead><tr>'
    result += ''.join(f'<th scope="col">{e(v)}</th>' for v in rows[0]) + '</tr></thead><tbody>'
    for row in rows[1:]:
        result += '<tr>'
        for v in row + [''] * (width-len(row)):
            result += '<td>' + (link(v,'Source') if v.startswith('https://') else e(v if v != '' else 'Not reported')) + '</td>'
        result += '</tr>'
    return result + '</tbody></table></div>'
sections=[('start','Start here'),('foundations','Architecture & types'),('guides','Technical guides'),('catalogue','A–Z catalogue'),('usage','Usage & satisfaction'),('geography','Geography & demographics'),('methodology','Methodology & coverage'),('sources','Sources & provenance')]
parts=['''<section class="wrap section language-library framework-library" aria-labelledby="framework-title"><div class="docs-heading"><p class="eyebrow">DEVELOPER FIELD GUIDE / 02</p><h1 class="page-title" id="framework-title">Web frameworks<br>&amp; technologies.</h1><p>From interface to infrastructure. Understand what each layer does.</p><div class="docs-stats"><span><strong>631</strong> technologies</span><span><strong>12</strong> technical guides</span><span><strong>26</strong> survey usage rows</span></div></div><div class="docs-layout"><aside class="docs-sidebar"><details open><summary>On this page</summary><nav aria-label="Web technologies documentation">''']
parts += [f'<a href="#{key}">{title}</a>' for key,title in sections]
parts.append('</nav><details><summary>Technical guides / 12</summary><nav aria-label="Framework technical guides">')
parts += [f'<a href="#guide-{i}">{e(g["name"])}</a>' for i,g in enumerate(guides)]
parts.append('</nav></details><details><summary>Catalogue / A–Z</summary><nav class="docs-alphabet" aria-label="Catalogue letters">')
initials=list(dict.fromkeys(r['initial'] for r in data['frameworks']))
parts += [f'<a href="#letter-{i}">{e(letter)}</a>' for i,letter in enumerate(initials)]
parts.append('''</nav></details></details><a class="text-link" href="/programming-languages/">Programming languages →</a><a class="text-link" href="/dashboard/">← Member workspace</a><a class="text-link" href="/topics/">All 22 topics →</a></aside><div class="docs-content">
<section id="start" class="docs-section"><p class="eyebrow">01 / ORIENTATION</p><h2>Understand the layer before the label.</h2><p>A UI library, a server framework and a runtime solve different problems. This directory preserves all 631 technologies in the supplied research pack dated <time datetime="2026-09-23">23 September 2026</time>, including historical entries. Inclusion does not establish current maintenance or suitability for a new project.</p><div class="docs-callout"><strong>Different measures. Different questions.</strong><p>Developer-reported use, survey satisfaction and detected website deployments have different denominators. Imported numerical snapshots are attributed to the supplied pack and have not all been independently revalidated. They are not one universal market-share ranking.</p></div></section>
<section id="foundations" class="docs-section"><p class="eyebrow">02 / ARCHITECTURE</p><h2>Where does the technology fit?</h2>
<details open><summary>Library, framework, runtime, CMS or generator?</summary><p>A library supplies building blocks that your code calls. A framework supplies conventions and often controls request or rendering flow. A runtime executes code and exposes platform APIs. A CMS manages content and editorial workflows. A static-site generator builds deployable pages ahead of requests. These roles can overlap; the catalogue retains its original classification alongside the technical guides.</p><p>React is a UI library; Node.js is a runtime; WordPress is a CMS. Treating all three as interchangeable framework choices hides the actual architectural decision.</p></details>
<details><summary>Client, server and full-stack boundaries</summary><p>Client code runs in an environment controlled by the user. It can improve interaction but cannot be trusted to authorize sensitive operations. Server code validates requests, checks permissions and accesses protected services. Full-stack tools connect these environments; they do not remove the boundary between them.</p></details>
<details><summary>Rendering: CSR, SSR, static generation and hydration</summary><p>Client-side rendering creates views in the browser. Server-side rendering produces HTML during request handling. Static generation produces it during a build. Hydration attaches client behavior to existing HTML; islands limit that behavior to selected components. Consider cacheability, freshness, accessibility and transferred JavaScript, not just initial appearance.</p></details>
<details><summary>The request lifecycle</summary><p>A typical server request passes through routing, middleware, authentication, authorization, validation and application logic before producing a response. Database transactions, timeouts and error mapping belong in that design. Background jobs need retries, idempotency and observability; successful queue submission does not mean the job succeeded.</p></details>
<details><summary>How to evaluate a framework</summary><ol><li>Define deployment targets, team knowledge and the product’s data boundaries.</li><li>Prototype an awkward real workflow, including failures and permissions.</li><li>Measure representative page weight, latency and database queries.</li><li>Check official support policies, upgrade paths and dependency maintenance.</li><li>Document migrations, backups, logging and recovery responsibilities.</li></ol><p>Use the research tables as context. Before choosing a dependency, review its current primary documentation and release history.</p></details></section>
<section id="guides" class="docs-section"><p class="eyebrow">03 / TECHNICAL FIELD GUIDES</p><h2>Roles, mechanics and tradeoffs.</h2><p>These 12 editorial guides describe typical engineering work, not measured occupational or demographic shares. The complete imported catalogue follows.</p>''')
for i,g in enumerate(guides):
    parts.append(f'<details class="language-guide" id="guide-{i}"><summary><strong>{e(g["name"])}</strong><span>{e(g["type"])}</span></summary><div class="guide-body">')
    for key,title in [('role','Role & typical users'),('technical','Technical model'),('tradeoffs','Engineering considerations'),('exercise','Try building')]:
        parts.append(f'<h3>{e(title)}</h3><p>{e(g[key])}</p>')
    parts.append('<p>'+link(g['source'],'Primary documentation')+'</p></div></details>')
parts.append('''</section><section id="catalogue" class="docs-section"><p class="eyebrow">04 / COMPLETE RESEARCH INDEX</p><h2>Explore all 631 technologies.</h2><p>Search by name, ecosystem, language or type. Expand an entry for its original classification, available metrics and coverage notes. Missing values mean “Not reported”, not zero use.</p><div class="docs-search"><label for="catalogue-search">Find a framework or technology</label><input id="catalogue-search" type="search" maxlength="120" placeholder="Try Django, Python, runtime or CSS" disabled aria-describedby="catalogue-count"><p id="catalogue-count" role="status">631 catalogue entries · search becomes available when the runtime loads.</p></div><p id="catalogue-empty" hidden>No matching technologies. Try a shorter query or clear your search.</p>''')
fields=[('category','Category'),('layer','Layer'),('kind','Type · source classification'),('primary_language','Primary language'),('ecosystem','Ecosystem'),('stackoverflow_rank_2025','Stack Overflow rank · 2025'),('stackoverflow_used_pct_2025','Stack Overflow used (%) · 2025'),('stateofjs_used_pct_2025','State of JS used (%) · 2025'),('stateofjs_satisfaction_pct_2025','State of JS satisfaction (%) · 2025'),('w3techs_all_websites_pct_2026_09_23','W3Techs all websites (%) · 23 Sep 2026'),('w3techs_js_library_market_share_pct_2026_09_23','W3Techs JS-library market (%) · 23 Sep 2026')]
for i,letter in enumerate(initials):
    parts.append(f'<details class="catalogue-group" id="letter-{i}" open><summary>{e(letter)}</summary><div class="catalogue-entries">')
    for r in data['frameworks']:
        if r['initial']!=letter:continue
        search=' | '.join(r[k] for k in ('name','category','layer','kind','primary_language','ecosystem'))
        if len(search.encode('utf-8')) >= 512:raise ValueError('Catalogue search buffer exceeded')
        parts.append(f'<details class="catalogue-entry" id="framework-{r["catalog_id"]}" data-catalogue-name="{e(search)}"><summary><span>{e(r["name"])}</span><small>{e(r["ecosystem"])}</small></summary><div class="guide-body"><dl class="language-metrics">')
        for key,label in fields:
            v=r[key]
            parts.append(f'<div><dt>{e(label)}</dt><dd>{e("Not reported" if v is None or v == "" else v)}</dd></div>')
        parts.append('</dl>')
        for key,label in [('notes','Research notes'),('country_breakdown_status','Country coverage'),('gender_breakdown_status','Gender coverage')]:
            parts.append(f'<p><strong>{label}:</strong> {e(r[key] or "No additional notes in the pack.")}</p>')
        parts.append('<p>'+link(r['catalog_source'],'Catalogue source')+' · <a href="#methodology">What these metrics mean</a></p>')
        for gi,g in enumerate(guides):
            if g['name']==r['name'] or (g['name']=='Ruby on Rails' and r['name']=='Rails'):
                parts.append(f'<a class="text-link" href="#guide-{gi}">Read the technical guide →</a>')
        parts.append('</div></details>')
    parts.append('</div></details>')
parts.append('</section><section id="usage" class="docs-section"><p class="eyebrow">05 / MEASUREMENT</p><h2>Usage, sentiment and deployments.</h2>')
for name,title,note in [
 ('Survey usage','Stack Overflow · 2025 reported use','26 rows; 23,678 respondents to the web-frameworks-and-technologies question. This multi-select survey includes runtimes and libraries. Its percentages are not mutually exclusive.'),
 ('JavaScript sentiment','State of JavaScript · 2025','20 selected technologies from the workbook. Use and satisfaction describe different survey responses and denominators. Missing use values remain missing even when satisfaction is available.'),
 ('Website detection','W3Techs · 23 September 2026','Three technologies. All-websites share and JavaScript-library market share use different denominators. Website detection does not count developers. Preserve the source’s category definitions, including how it groups Angular-related technologies; do not infer version-specific adoption.')]:
    parts.append(f'<details open><summary>{e(title)}</summary><p>{e(note)}</p>'+table(workbook[name],title)+'</details>')
parts.append('''</section><section id="geography" class="docs-section"><p class="eyebrow">06 / PEOPLE &amp; PLACE</p><h2>Describe the sample honestly.</h2><p>The 146 country and region rows describe Stack Overflow survey respondents. They are not framework use by country, national developer populations or a basis for attributing a technology to a demographic group.</p><details><summary>All 146 survey geography rows</summary>'''+table(workbook['Survey geography'],'Stack Overflow 2025 · respondent geography')+'''</details><h3>Gender breakdowns are unavailable</h3><p>The pack does not contain verified framework-by-gender cross-tabs. A future analysis would need explicit respondent-level data, sample sizes, year, non-response handling and the original categories, including non-binary and undisclosed responses. No percentages are inferred here.</p></section>
<section id="methodology" class="docs-section"><p class="eyebrow">07 / COVERAGE &amp; LIMITATIONS</p><h2>Read the methodology.</h2><p>The catalogue is broad rather than exhaustive. Its source classifications are retained, including broad or combined type labels. Detailed guides clarify the tools they cover. Catalogue counts describe this pack, not market size.</p>''')
for name in ('Dashboard','Catalogue breakdown','Methodology'):
    rows=workbook[name]
    if name=='Dashboard':
        parts.append('<details><summary>Workbook overview</summary>'+''.join('<p>'+e(' · '.join(row))+'</p>' for row in rows)+'</details>')
    else:parts.append('<details><summary>'+e(name)+'</summary>'+table(rows,name)+'</details>')
parts.append('</section><section id="sources" class="docs-section"><p class="eyebrow">08 / PROVENANCE</p><h2>Follow the sources.</h2><p>All 21 source records are retained. The original workbook, JSON, CSVs and README are preserved in the repository. Numerical values are imported snapshots, not live counters. Source licensing and attribution remain applicable; this page does not reproduce linked documentation in full.</p>')
for r in data['sources']:
    parts.append('<article class="research-source"><h3>'+e(r['dataset'])+'</h3><p>'+link(r['url'],r['source'])+'</p><p>'+e(r['notes'])+'</p></article>')
parts.append('</section><a class="text-link" href="#framework-title">Back to top ↑</a></div></div></section>')
(ROOT / 'web/pages/web-frameworks.html').write_text('\n'.join(parts)+'\n',encoding='utf-8')
print('Built web frameworks documentation: 631 entries, 12 guides, all workbook research tables.')
