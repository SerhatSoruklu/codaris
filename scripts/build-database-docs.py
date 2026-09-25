#!/usr/bin/env python3
"""Render all supplied database research without runtime dependencies."""
import html
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'data/databases'
data = json.loads((DATA / 'codaris_databases_catalog.json').read_text(encoding='utf-8'))
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
sections=[('start','Start here'),('foundations','Data models & architecture'),('guides','Technical guides'),('catalogue','A–Z catalogue'),('usage','Usage & popularity'),('platforms','Managed services & platforms'),('geography','Geography & demographics'),('methodology','Methodology & coverage'),('sources','Sources & provenance')]
parts=['''<section class="wrap section language-library database-library" aria-labelledby="database-title"><div class="docs-heading"><p class="eyebrow">DEVELOPER FIELD GUIDE / 03</p><h1 class="page-title" id="database-title">Databases<br>&amp; data platforms.</h1><p>Model the data. Understand the guarantees. Plan for recovery.</p><div class="docs-stats"><span><strong>643</strong> catalogue entries</span><span><strong>12</strong> technical guides</span><span><strong>24</strong> platform profiles</span></div></div><div class="docs-layout"><aside class="docs-sidebar"><details open><summary>On this page</summary><nav aria-label="Database documentation">''']
parts += [f'<a href="#{key}">{title}</a>' for key,title in sections]
parts.append('</nav><details><summary>Technical guides / 12</summary><nav aria-label="Database technical guides">')
parts += [f'<a href="#guide-{i}">{e(g["name"])}</a>' for i,g in enumerate(guides)]
parts.append('</nav></details><details><summary>Catalogue / A–Z</summary><nav class="docs-alphabet" aria-label="Catalogue letters">')
initials=list(dict.fromkeys(r['initial'] for r in data['databases']))
parts += [f'<a href="#letter-{i}">{e(letter)}</a>' for i,letter in enumerate(initials)]
parts.append('''</nav></details></details><a class="text-link" href="/programming-languages/">Programming languages →</a><a class="text-link" href="/web-frameworks/">Web technologies →</a><a class="text-link" href="/dashboard/">← Member workspace</a><a class="text-link" href="/topics/">All 22 topics →</a></aside><div class="docs-content">
<section id="start" class="docs-section"><p class="eyebrow">01 / ORIENTATION</p><h2>Choose for the workload and its invariants.</h2><p>This library preserves all 643 entries from the supplied research pack dated <time datetime="2026-09-23">23 September 2026</time>. It includes engines, embedded databases, warehouses, search and vector systems, managed services and developer platforms. They operate at different architectural layers and are not interchangeable.</p><div class="docs-callout"><strong>A popularity score is not a correctness guarantee.</strong><p>Stack Overflow reported use and DB-Engines popularity measure different things. Imported numerical snapshots have not all been independently revalidated. A catalogue listing does not establish current maintenance, suitability or deployment share.</p></div></section>
<section id="foundations" class="docs-section"><p class="eyebrow">02 / ENGINEERING FOUNDATIONS</p><h2>Understand what the database promises.</h2>
<details open><summary>Relational, document, key-value and wide-column models</summary><p>Relational systems model records and relationships through tables, keys and constraints. Document systems group related values into documents. Key-value stores organize access around keys. Wide-column systems commonly use partition and clustering keys for distributed access patterns. Features overlap, so evaluate the actual engine rather than relying only on the category.</p></details>
<details><summary>Graph, time-series, search and vector systems</summary><p>Graph queries emphasize relationships and traversal. Time-series workloads emphasize timestamped observations and retention. Search systems build indexes for retrieval and relevance. Vector systems retrieve by similarity in an embedding space. These may be standalone databases or capabilities added to another engine; decide where authoritative data lives and how derived indexes are rebuilt.</p></details>
<details><summary>OLTP, analytics and embedded deployment</summary><p>Transactional workloads often involve many small reads and writes with concurrency requirements. Analytical workloads often scan and aggregate larger datasets. Embedded databases run in the application's process; client/server databases expose a service boundary. Warehouses and lakehouse platforms introduce additional storage, compute and governance choices.</p></details>
<details><summary>Transactions, isolation and consistency</summary><p>Atomicity groups changes into an all-or-nothing unit. Constraints protect valid states. Isolation describes interactions between concurrent transactions; durability concerns acknowledged changes surviving failures under stated assumptions. Distributed read consistency is a related but separate question. Define the anomalies your application can tolerate and test the database's documented behavior.</p></details>
<details><summary>Indexes, query plans and partition keys</summary><p>An index can reduce read work at the cost of storage and write maintenance. Query plans reveal execution choices; measure with representative data and parameter values. Partition keys affect data distribution and hot spots. Adding indexes or partitions without measuring the workload can increase complexity without improving latency.</p></details>
<details><summary>Replication is not a backup strategy</summary><p>A replica can repeat an accidental deletion or unwanted change. Define recovery point and recovery time objectives, retain appropriate backups, and rehearse restoration. Include credentials, migrations, encryption keys and application compatibility in recovery planning. Test degraded operation and failover, not only healthy throughput.</p></details>
<details><summary>Secure application boundaries</summary><p>Bind user-controlled values through parameterized APIs. Authorize operations before accessing protected data, give services only the permissions they need and keep database credentials outside browser code. Constraints protect data integrity; they do not replace application authorization. CODARIS accesses PostgreSQL through libpq from the native C backend.</p></details>
<details><summary>A practical evaluation checklist</summary><ol><li>Write down entities, relationships and durable invariants.</li><li>List real queries, write rates, contention and retention requirements.</li><li>Prototype transactions and failure scenarios with representative data.</li><li>Measure query plans, resource use and operational costs.</li><li>Verify restoration, migration and upgrade procedures before committing.</li></ol></details></section>
<section id="guides" class="docs-section"><p class="eyebrow">03 / TECHNICAL FIELD GUIDES</p><h2>Roles, mechanics and tradeoffs.</h2><p>Twelve editorial starting points with primary documentation links. Typical users describe engineering roles, not measured demographic shares. Remaining catalogue entries retain their original data and source notes.</p>''')
for i,g in enumerate(guides):
    parts.append(f'<details class="language-guide" id="guide-{i}"><summary><strong>{e(g["name"])}</strong><span>{e(g["type"])}</span></summary><div class="guide-body">')
    for key,title in [('role','Role & typical users'),('technical','Technical model'),('tradeoffs','Engineering considerations'),('exercise','Try building')]:
        parts.append(f'<h3>{e(title)}</h3><p>{e(g[key])}</p>')
    parts.append('<p>'+link(g['source'],'Primary documentation')+'</p></div></details>')
parts.append('''</section><section id="catalogue" class="docs-section"><p class="eyebrow">04 / COMPLETE RESEARCH INDEX</p><h2>Explore all 643 entries.</h2><p>Search by name, data model, product type, deployment or provider. Expand an entry to inspect the source classification and available metrics. Missing values mean “Not reported”, not zero use.</p><div class="docs-search"><label for="catalogue-search">Find a database or data platform</label><input id="catalogue-search" type="search" maxlength="120" placeholder="Try PostgreSQL, graph, embedded or managed" disabled aria-describedby="catalogue-count"><p id="catalogue-count" role="status">643 catalogue entries · search becomes available when the runtime loads.</p></div><p id="catalogue-empty" hidden>No matching databases or platforms. Try a shorter query or clear your search.</p>''')
fields=[('product_type','Product type'),('primary_model','Primary model'),('secondary_models','Secondary models'),('deployment','Deployment'),('provider_or_project','Provider / project'),('stackoverflow_rank_2025','Stack Overflow rank · 2025'),('stackoverflow_used_pct_2025','Stack Overflow used (%) · 2025'),('dbengines_rank_2026_09','DB-Engines rank · September 2026'),('dbengines_score_2026_09','DB-Engines score · September 2026'),('dbengines_top20_model','DB-Engines top-20 model')]
for i,letter in enumerate(initials):
    parts.append(f'<details class="catalogue-group" id="letter-{i}" open><summary>{e(letter)}</summary><div class="catalogue-entries">')
    for r in data['databases']:
        if r['initial']!=letter:continue
        search=' | '.join(r[k] for k in ('name','primary_model','secondary_models','product_type','deployment','provider_or_project'))
        if len(search.encode('utf-8')) >= 512:raise ValueError('Catalogue search buffer exceeded')
        parts.append(f'<details class="catalogue-entry" id="database-{r["catalog_id"]}" data-catalogue-name="{e(search)}"><summary><span>{e(r["name"])}</span><small>{e(r["product_type"])}</small></summary><div class="guide-body"><dl class="language-metrics">')
        for key,label in fields:
            v=r[key]
            parts.append(f'<div><dt>{e(label)}</dt><dd>{e("Not reported" if v is None or v == "" else v)}</dd></div>')
        parts.append('</dl>')
        for key,label in [('notes','Research notes'),('country_breakdown_status','Country coverage'),('gender_breakdown_status','Gender coverage')]:
            parts.append(f'<p><strong>{label}:</strong> {e(r[key] or "No additional notes in the pack.")}</p>')
        parts.append('<p>'+link(r['source_url'],'Catalogue source')+' · <a href="#methodology">What these metrics mean</a></p>')
        for gi,g in enumerate(guides):
            if g['name']==r['name']:
                parts.append(f'<a class="text-link" href="#guide-{gi}">Read the technical guide →</a>')
        parts.append('</div></details>')
    parts.append('</div></details>')
parts.append('</section><section id="usage" class="docs-section"><p class="eyebrow">05 / DISTINCT MEASURES</p><h2>Developer use and popularity.</h2>')
for name,title,note in [
 ('Developer usage','Stack Overflow · 2025 database use','16 rows; 26,083 respondents to the database-environment question. Multiple selections are possible, so percentages overlap. These describe survey respondents rather than every developer.'),
 ('DB-Engines top 20','DB-Engines · September 2026 top 20','Popularity-index scores are not percentages, measured usage or deployment counts. These are the pack’s dated values, not a live ranking.'),
 ('Model rankings','DB-Engines · model categories','Nine category rows, including the overall ranking. The pack reports 438 systems in that ranking versus 643 entries in this broader directory. Different catalogue boundaries explain the difference; do not add overlapping categories as if they were disjoint populations.')]:
    parts.append(f'<details open><summary>{e(title)}</summary><p>{e(note)}</p>'+table(workbook[name],title)+'</details>')
parts.append('''</section><section id="platforms" class="docs-section"><p class="eyebrow">06 / LAYERS &amp; RESPONSIBILITIES</p><h2>An engine is not the whole platform.</h2><p>A managed service runs or packages database capabilities. A developer platform may also expose authentication, APIs, storage or realtime features. Compare the underlying data model and guarantees separately from hosting and application services.</p><p>The 24 source-pack profiles below retain their dated notes. Product offerings can change; check each linked provider before making a deployment decision. MongoDB and MongoDB Atlas, for example, represent an engine and a managed platform rather than two interchangeable data models.</p><details><summary>All 24 developer-platform profiles</summary>'''+table(workbook['Developer platforms'],'Developer platforms · source-pack profiles')+'''</details></section>
<section id="geography" class="docs-section"><p class="eyebrow">07 / PEOPLE &amp; PLACE</p><h2>Understand who the data represents.</h2><p>The 146 country and region rows describe overall Stack Overflow survey respondents. They are not database usage by country or national developer populations.</p><details><summary>All 146 survey geography rows</summary>'''+table(workbook['Survey geography'],'Stack Overflow 2025 · respondent geography')+'''</details><h3>No inferred gender percentages</h3><p>The pack contains no verified database-by-gender cross-tabs. Any future analysis needs direct supporting data, year, sample sizes, uncertainty and the source’s non-binary, undisclosed and missing categories. Database choice does not identify an individual’s demographic group.</p></section>
<section id="methodology" class="docs-section"><p class="eyebrow">08 / COVERAGE &amp; LIMITATIONS</p><h2>Read the methodology.</h2><p>This broad catalogue is not an exhaustive list of every database ever built. Source labels are preserved even when broad or combined; the technical guides provide additional context for their covered products.</p>''')
for name in ('Dashboard','Methodology'):
    rows=workbook[name]
    if name=='Dashboard':parts.append('<details><summary>Workbook overview</summary>'+''.join('<p>'+e(' · '.join(row))+'</p>' for row in rows)+'</details>')
    else:parts.append('<details><summary>'+e(name)+'</summary>'+table(rows,name)+'</details>')
parts.append('</section><section id="sources" class="docs-section"><p class="eyebrow">09 / PROVENANCE</p><h2>Follow the evidence.</h2><p>All 21 source records are retained. Original JSON, CSVs, workbook and README are preserved in the repository. Imported numerical values are attributed snapshots, not live counters. Source licensing and attribution remain applicable; linked documentation is not reproduced in full.</p>')
for r in data['sources']:
    parts.append('<article class="research-source"><h3>'+e(r['dataset'])+'</h3><p>'+link(r['url'],r['source'])+'</p><p>'+e(r['notes'])+'</p></article>')
parts.append('</section><a class="text-link" href="#database-title">Back to top ↑</a></div></div></section>')
(ROOT / 'web/pages/databases.html').write_text('\n'.join(parts)+'\n',encoding='utf-8')
print('Built database documentation: 643 entries, 12 guides, all workbook research tables.')
