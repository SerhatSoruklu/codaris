#!/usr/bin/env python3
"""Build owner-supplied research topics and editorial learning guides; stdlib only."""
import html
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'data/topics'
e=lambda v:html.escape(str(v),quote=True)
def label(k):
    return k.replace('_',' ').replace(' pct',' (%)').replace('2026 09','Sep 2026').replace('stateofjs','State of JS').replace('stackoverflow','Stack Overflow').replace('headline','Source-pack headline').capitalize()
def val(v):
    if v is None or v=='':return 'Not reported'
    if isinstance(v,(list,dict)):return e(json.dumps(v,ensure_ascii=False))
    if isinstance(v,str) and v.startswith('https://') and not any(c.isspace() for c in v):return f'<a href="{e(v)}" target="_blank" rel="noopener noreferrer">{e(v)} ↗</a>'
    return e(v)
def fields(row):
    return '<dl class="language-metrics">'+''.join(f'<div><dt>{e(label(k))}</dt><dd>{val(v)}</dd></div>' for k,v in row.items())+'</dl>'
def table(rows,caption):
    if not rows:return '<p>No measurements supplied for this table. This is missing coverage, not zero adoption.</p>'
    width=max(map(len,rows));head=rows[0]+['']*(width-len(rows[0]))
    return f'<div class="research-table" tabindex="0" role="region" aria-label="{e(caption)}"><table><caption>{e(caption)}</caption><thead><tr>'+''.join(f'<th scope="col">{e(v or "Additional detail")}</th>' for v in head)+'</tr></thead><tbody>'+''.join('<tr>'+''.join('<td>'+val(v)+'</td>' for v in row+['']*(width-len(row)))+'</tr>' for row in rows[1:])+'</tbody></table></div>'
def dict_table(rows,title):
    if not rows:return table([],title)
    keys=list(dict.fromkeys(k for row in rows for k in row))
    return table([[label(k) for k in keys]]+[[row.get(k) for k in keys] for row in rows],title)
def start(title,number,intro,audience,flow,nav,stats):
    return [f'<section class="wrap section language-library topic-library" aria-labelledby="topic-title"><div class="docs-heading"><p class="eyebrow">DEVELOPER LIBRARY / {number}</p><h1 class="page-title" id="topic-title">{e(title)}.</h1><p>{e(intro)}</p><div class="docs-stats">'+''.join(f'<span><strong>{e(value)}</strong>{e(name)}</span>' for value,name in stats)+'</div></div><div class="docs-layout"><aside class="docs-sidebar"><details open><summary>On this page</summary><nav aria-label="Topic documentation">'+''.join(f'<a href="#{key}">{e(name)}</a>' for key,name in nav)+'</nav></details><a class="text-link" href="/dashboard/">← Member workspace</a><a class="text-link" href="/topics/">All 22 topics →</a></aside><div class="docs-content">',f'<section class="docs-section" id="orientation"><p class="eyebrow">ORIENTATION</p><h2>Roles and responsibilities.</h2><p>{e(audience)}</p><h3>How the pieces relate</h3><pre class="docs-code" tabindex="0">{e(flow)}</pre></section>']
def guide_sections(guides):
    out=['<section class="docs-section" id="guides"><p class="eyebrow">TECHNICAL GUIDES</p><h2>Mechanics, tradeoffs and practice.</h2><p>These are CODARIS editorial explanations. Occupational examples describe engineering responsibilities, not measured demographic shares.</p>']
    for i,g in enumerate(guides):
        out.append(f'<details class="language-guide" id="guide-{i}"><summary><strong>{e(g["title"])}</strong></summary><div class="guide-body"><h3>Technical model</h3><p>{e(g["model"])}</p><h3>Failure modes and decisions</h3><p>{e(g["pitfalls"])}</p><h3>Practice task</h3><p>{e(g["exercise"])}</p>')
        if g.get('code'):
            code=e(g['code']).replace('{','&#123;')
            out.append(f'<pre class="docs-code" tabindex="0"><code>{code}</code></pre>')
        out.append('<p>'+val(g['source'])+'</p></div></details>')
    out.append('</section>');return out

def research(manifest,editorial):
    directory=DATA/manifest['slug'];d=json.loads((directory/manifest['catalog']).read_text());key='runtimes' if 'runtimes' in d else 'catalogue';rows=d[key]
    assert len(rows)==manifest['count']
    guides=editorial['guides'];nav=[('orientation','Roles & architecture'),('guides','Technical guides'),('catalogue','Search the catalogue'),('research','Research tables'),('workbook','Workbook detail'),('methodology','Scope & demographics'),('sources','Sources & provenance')]
    parts=start(manifest['title'],f'{manifest["number"]:02d}',editorial['intro'],editorial['audience'],editorial['flow'],nav,[(len(rows),'catalogue entries'),(len(guides),'technical guides'),(len(d['sources']),'source records')])
    parts+=guide_sections(guides)
    parts.append(f'<section class="docs-section" id="catalogue"><p class="eyebrow">COMPLETE SOURCE-PACK INDEX</p><h2>Explore {len(rows):,} entries.</h2><p>Search by name, type, use or environment. Source classifications are preserved; historical inclusion does not establish current maintenance or suitability.</p><div class="docs-search"><label for="catalogue-search">Find a technology</label><input id="catalogue-search" type="search" maxlength="120" disabled placeholder="Search names, roles and platforms…" aria-describedby="catalogue-count"><p id="catalogue-count" role="status">{len(rows)} catalogue entries · search becomes available when the runtime loads.</p></div><p id="catalogue-empty" hidden>No matching entries. Try a shorter query or clear the search.</p>')
    initials=list(dict.fromkeys(r['initial'] for r in rows))
    parts.append('<nav class="docs-letter-links" aria-label="Catalogue letters">'+''.join(f'<a href="#letter-{i}">{e(letter)}</a>' for i,letter in enumerate(initials))+'</nav>')
    for i,letter in enumerate(initials):
        parts.append(f'<details class="catalogue-group" id="letter-{i}" open><summary>{e(letter)}</summary><div class="catalogue-entries">')
        for row in rows:
            if row['initial']!=letter:continue
            search=' | '.join(str(row.get(k) or '') for k in ('name','class','subtype','primary_use','ecosystem_or_vendor','deployment_or_interface','runtime_family','runtime_type','languages_or_code','typical_environment','provider_or_project'))
            assert len(search.encode())<4096
            parts.append(f'<details class="catalogue-entry" id="entry-{row["catalog_id"]}" data-catalogue-name="{e(search)}"><summary><span>{e(row["name"])}</span><small>{e(row.get("class",row.get("runtime_type","")))}</small></summary><div class="guide-body">'+fields(row)+'<p>Attribution comes from the supplied pack and may describe a category rather than independently verify this individual entry. <a href="#methodology">Read the scope notes</a>.</p></div></details>')
        parts.append('</div></details>')
    parts.append('</section><section class="docs-section" id="research"><p class="eyebrow">SOURCE-SPECIFIC EVIDENCE</p><h2>Keep the denominator attached.</h2><p>Imported figures are dated research snapshots, not live measurements or a universal market-share score. Missing values remain unavailable. The original sources, dates, populations and approximation notes are retained; not every numerical claim has been independently revalidated.</p>')
    for k,v in d.items():
        if k in (key,'metadata','sources'):continue
        parts.append('<details><summary>'+e(label(k))+'</summary>'+dict_table(v,label(k))+'</details>')
    parts.append('</section><section class="docs-section" id="workbook"><p class="eyebrow">COMPLETE WORKBOOK</p><h2>Inspect the underlying tables.</h2><p>All non-empty source workbook cells are preserved below, including supplementary relationships, breakdowns and notes. Catalogue sheets repeat the searchable index above in the original column layout.</p>')
    sheets=json.loads((directory/'workbook-tables.json').read_text())
    for title,values in sheets.items():parts.append('<details><summary>'+e(title.replace('_',' '))+'</summary>'+table(values,title)+'</details>')
    parts.append('</section><section class="docs-section" id="methodology"><p class="eyebrow">RESEARCH INTEGRITY</p><h2>Scope and demographic limits.</h2>'+fields(d['metadata'])+'<p>Survey respondents are not a census of all developers. A country’s overall respondent share cannot establish tool use within that country. No per-tool gender percentages are inferred; defensible cross-tabs need direct evidence, sample sizes, year and the source’s complete response categories.</p><p>These categories overlap. A tool can serve several roles, so total catalogue rows do not equal globally unique technologies. Version and product-status notes describe the supplied snapshot date; check primary documentation before adopting or upgrading.</p></section><section class="docs-section" id="sources"><p class="eyebrow">PROVENANCE</p><h2>Follow the research.</h2><p>Original JSON, CSVs, workbook and README are preserved in the repository. Their checksums are recorded in the companion SHA256SUMS integrity manifest. Source terms and attribution remain applicable. Editorial guides are separate from imported research.</p>')
    for source in d['sources']:parts.append('<article class="research-source">'+fields(source)+'</article>')
    for readme in directory.glob('README*.md'):parts.append('<details><summary>Original source-pack README</summary><pre class="docs-source-note">'+e(readme.read_text())+'</pre></details>')
    parts.append('</section><a class="text-link" href="#topic-title">Back to top ↑</a></div></div></section>')
    (ROOT/'web/pages'/f'{manifest["slug"]}.html').write_text('\n'.join(parts)+'\n')

manifests=json.loads((DATA/'manifest.json').read_text())
editorial=json.loads((DATA/'editorial.json').read_text())
assert set(editorial)=={m['slug'] for m in manifests}, 'Every research pack needs editorial guides'
for manifest in manifests:
    research(manifest,editorial[manifest['slug']])
print(f'Built {len(editorial)} detailed research topic pages.')

foundations=json.loads((DATA/'foundations.json').read_text())
for f in foundations:
    nav=[('orientation','Roles & workflow'),('guides','Six practical chapters'),('project','Practice project'),('reading','Further reading')]
    parts=start(f['title'],'FOUNDATIONS',f['intro'],f['audience'],f['flow'],nav,[(6,'practical chapters'),(1,'practice project'),('Code','worked examples')])
    parts+=guide_sections(f['guides'])
    parts.append('<section class="docs-section" id="project"><p class="eyebrow">APPLY WHAT YOU LEARN</p><h2>Build and verify.</h2><p>'+e(f['project'])+'</p><h3>Evidence to bring to a review</h3><ul><li>A precise description of the behavior and its boundaries.</li><li>Tests or observations covering success, invalid input and dependency failure.</li><li>A short explanation of one tradeoff and a known limitation.</li><li>Instructions that let another developer reproduce the result.</li></ul><p>Examples illustrate individual ideas and are not a complete production application. Run experiments in a disposable environment and adapt them to the documented versions of your tools.</p></section><section class="docs-section" id="reading"><h2>Continue with primary documentation.</h2><p>These guides were researched against primary documentation on 23 September 2026. They do not include invented popularity or demographic statistics.</p>')
    for g in f['guides']:parts.append('<p>'+val(g['source'])+'</p>')
    parts.append('</section><a class="text-link" href="#topic-title">Back to top ↑</a></div></div></section>')
    (ROOT/'web/pages'/f'{f["slug"]}.html').write_text('\n'.join(parts)+'\n')

existing=[
 {'slug':'programming-languages','title':'Programming Languages','count':674,'topic_id':6,'description':'Language roles, technical models, tradeoffs and research snapshots.'},
 {'slug':'web-frameworks','title':'Web Frameworks & Technologies','count':631,'topic_id':7,'description':'UI libraries, runtimes, frameworks and full-stack architecture.'},
 {'slug':'databases','title':'Databases & Data Platforms','count':643,'topic_id':8,'description':'Data models, transactions, indexing, deployment and recovery.'}]
research_topics=existing+[{'slug':m['slug'],'title':m['title'],'count':m['count'],'topic_id':m['number']+5,'description':editorial[m['slug']]['intro']} for m in manifests]
foundation_topics=[{'slug':f['slug'],'title':f['title'],'topic_id':i,'description':f['intro']} for i,f in enumerate(foundations)]
topics=research_topics+foundation_topics
assert len(topics)==22 and {t['topic_id'] for t in topics}==set(range(22))
(ROOT/'data/topics/library.json').write_text(json.dumps(topics,ensure_ascii=False,indent=2)+'\n')
def cards(entries,tracking):
    output=['<div class="learning-grid topic-library-grid">']
    icons={
        'programming-languages':'code','web-frameworks':'panels-top-left','databases':'database',
        'runtimes':'cpu','package-build-tools':'package','developer-environments':'monitor-cog',
        'version-control':'git-branch','testing-qa':'flask-conical','cloud-hosting':'cloud',
        'containers-infrastructure':'container','cicd-automation':'workflow','servers-networking':'network',
        'messaging-streaming':'radio','observability':'activity','apis-auth-integration':'plug',
        'ai-developer-tools':'bot','software-development':'code','system-design':'boxes',
        'python':'code','angular':'panels-top-left','web-foundations':'panels-top-left',
        'sql-postgresql':'database',
    }
    for t in entries:
        desc=t['description'].split('. ')[0].rstrip('.')+'.'
        tag=f'{t["count"]:,} catalogue entries' if 'count' in t else '6 chapters · worked examples'
        icon=icons[t['slug']]
        output.append(f'<article class="account-card learning-card"><p class="eyebrow">{e(tag)}</p><h3><svg class="icon" aria-hidden="true" focusable="false"><use href="/assets/icons/lucide.svg#{icon}"></use></svg><span>{e(t["title"])}</span></h3><p class="topic-description">{e(desc)}</p><div class="topic-actions"><a class="button topic-open" href="/{t["slug"]}/" aria-label="View topic: {e(t["title"])}">View topic <svg class="icon icon--sm" aria-hidden="true" focusable="false"><use href="/assets/icons/lucide.svg#arrow-right"></use></svg></a>')
        if tracking:output.append(f'<button type="button" class="topic-toggle" data-topic="{t["topic_id"]}" aria-pressed="false" data-preview-button disabled>Mark as read</button>')
        output.append('</div></article>')
    output.append('</div>');return '\n'.join(output)
learning='''<section id="panel-learn" role="tabpanel" aria-labelledby="tab-learn" tabindex="0" hidden>
<div class="account-heading learning-heading"><div><p class="eyebrow">THE DEVELOPER LIBRARY</p><h2>Choose your next chapter.</h2><p>22 populated topics. Read the research, understand the tradeoffs, then put an idea into practice.</p></div><p id="learning-progress" role="status">0 of 22 topics read</p></div>
<div class="topic-search"><label for="topic-search-input"><svg class="icon" aria-hidden="true" focusable="false"><use href="/assets/icons/lucide.svg#search"></use></svg><span>Find a topic</span></label><div class="topic-search-control"><input id="topic-search-input" type="search" maxlength="100" placeholder="Search languages, databases, security…" autocomplete="off" aria-describedby="topic-search-hint topic-search-count" aria-controls="panel-learn"><button type="button" id="topic-search-clear" aria-label="Clear topic search" title="Clear search" hidden><svg class="icon" aria-hidden="true" focusable="false"><use href="/assets/icons/lucide.svg#x"></use></svg></button></div><div class="topic-search-meta"><small id="topic-search-hint">Search topic names and descriptions.</small><small id="topic-search-count" role="status" aria-live="polite">22 topics</small></div></div>
<div class="learning-overview"><span><strong>16</strong> research directories</span><span><strong>4,621</strong> catalogue rows, including overlaps</span><span><strong>6</strong> practical learning guides</span></div>
<p class="learning-note">Read markers last for this page visit. Open a topic to read its full guide; no account is needed to read the library.</p>
<h3 class="learning-group-title" data-topic-group-title>Technology research <span>01–16</span></h3>'''+cards(research_topics,True)+'<h3 class="learning-group-title" data-topic-group-title>Developer foundations <span>Six practical guides</span></h3>'+cards(foundation_topics,True)+'</section>'
v2_topics=json.loads((DATA/'v2-roadmap.json').read_text(encoding='utf-8'))
assert len(v2_topics)==50 and [topic['number'] for topic in v2_topics]==list(range(1,51))
v2_section=['''<section class="v2-roadmap" aria-labelledby="v2-roadmap-title"><header class="v2-roadmap-heading"><div><span class="v2-roadmap-badge"><span aria-hidden="true"></span> V2 ROADMAP / 50 TOPICS</span><p class="eyebrow">THE LIBRARY WILL KEEP GROWING</p><h2 id="v2-roadmap-title">More ground to explore.</h2><p>Topics V2 is planned to expand CODARIS into security, systems engineering, data infrastructure, networking, AI engineering and production reliability.</p><p class="v2-roadmap-note">These topics are planned for a future release. Their descriptions outline the intended coverage and may be refined as the library develops.</p></div><figure class="v2-roadmap-preview"><svg class="v2-library-diagram" viewBox="0 0 720 430" role="img" aria-labelledby="v2-library-title v2-library-desc" xmlns="http://www.w3.org/2000/svg"><title id="v2-library-title">CODARIS Topics V2 library structure</title><desc id="v2-library-desc">An original diagram showing Foundations, Advanced Engineering and CODARIS Research as three library levels, with working groups beneath research topics.</desc><defs><linearGradient id="v2-diagram-bg" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#10232e"/><stop offset="1" stop-color="#09131c"/></linearGradient><linearGradient id="v2-diagram-accent" x1="0" x2="1"><stop stop-color="#45d9ad"/><stop offset="1" stop-color="#59e4eb"/></linearGradient></defs><rect x="1" y="1" width="718" height="428" rx="18" fill="url(#v2-diagram-bg)" stroke="#35505e"/><text x="34" y="42" fill="#59e4eb" font-family="monospace" font-size="11" letter-spacing="2">CODARIS / DEVELOPER LIBRARY</text><text x="686" y="42" fill="#97a7b5" text-anchor="end" font-family="monospace" font-size="10">TOPIC ARCHITECTURE · V2</text><path d="M36 59H684" stroke="#263b48"/><g font-family="Arial,Helvetica,sans-serif"><rect x="38" y="82" width="190" height="106" rx="11" fill="#0e1c26" stroke="#3c6c71"/><rect x="38" y="82" width="190" height="3" rx="2" fill="#45d9ad"/><text x="58" y="113" fill="#45d9ad" font-family="monospace" font-size="10" letter-spacing="1.4">LEVEL 01</text><text x="58" y="143" fill="#edf3f7" font-size="20" font-weight="700">Foundations</text><text x="58" y="166" fill="#97a7b5" font-size="12">Core skills and shared concepts</text><rect x="265" y="82" width="190" height="106" rx="11" fill="#0e1c26" stroke="#3c6c71"/><rect x="265" y="82" width="190" height="3" rx="2" fill="#4acfc0"/><text x="285" y="113" fill="#59e4eb" font-family="monospace" font-size="10" letter-spacing="1.4">LEVEL 02</text><text x="285" y="139" fill="#edf3f7" font-size="16" font-weight="700">Advanced</text><text x="285" y="158" fill="#edf3f7" font-size="16" font-weight="700">Engineering</text><text x="285" y="177" fill="#97a7b5" font-size="11">Deeper systems and methods</text><rect x="492" y="82" width="190" height="106" rx="11" fill="#0e1c26" stroke="#3c6c71"/><rect x="492" y="82" width="190" height="3" rx="2" fill="#59e4eb"/><text x="512" y="113" fill="#59e4eb" font-family="monospace" font-size="10" letter-spacing="1.4">LEVEL 03</text><text x="512" y="143" fill="#edf3f7" font-size="20" font-weight="700">CODARIS Research</text><text x="512" y="166" fill="#97a7b5" font-size="12">Specialized technical areas</text></g><path d="M133 188V222H360M360 188V222M587 188V222H360V250" fill="none" stroke="url(#v2-diagram-accent)" stroke-width="2"/><circle cx="360" cy="222" r="4" fill="#59e4eb"/><rect x="222" y="250" width="276" height="102" rx="11" fill="#0b1720" stroke="#34616c"/><text x="360" y="282" text-anchor="middle" fill="#59e4eb" font-family="monospace" font-size="10" letter-spacing="1.5">COMMUNITY PRACTICE</text><text x="360" y="313" text-anchor="middle" fill="#edf3f7" font-family="Arial,Helvetica,sans-serif" font-size="20" font-weight="700">Working Groups</text><text x="360" y="335" text-anchor="middle" fill="#97a7b5" font-family="Arial,Helvetica,sans-serif" font-size="12">Form around research topics</text><path d="M360 352V377H178M360 377H542" fill="none" stroke="#35505e" stroke-width="2"/><rect x="76" y="377" width="204" height="32" rx="7" fill="#11232d" stroke="#35505e"/><text x="178" y="398" text-anchor="middle" fill="#b9f5e2" font-family="monospace" font-size="10">Topic-specific teams</text><rect x="440" y="377" width="204" height="32" rx="7" fill="#11232d" stroke="#35505e"/><text x="542" y="398" text-anchor="middle" fill="#b9f5e2" font-family="monospace" font-size="10">Specs · code · tests</text></svg><figcaption><span>THREE LEARNING LEVELS / WORKING GROUPS BELOW RESEARCH</span><span>CODARIS TOPICS V2</span></figcaption></figure></header><div class="v2-topic-list-heading"><h3>Planned topic list</h3><span>01–50 / PROPOSED SCOPE</span></div><ol class="v2-topic-list">''']
for topic in v2_topics:
    v2_section.append(f'<li><span class="v2-topic-number">{topic["number"]:02d}</span><div><h4>{e(topic["title"])}</h4><p>{e(topic["scope"])}</p></div></li>')
v2_section.append('</ol></section>')
learning=learning.replace('</section>', ''.join(v2_section)+'</section>')
(ROOT/'web/partials/learning-library.html').write_text(learning+'\n')
hub='<section class="wrap section account-page"><p class="eyebrow">CODARIS / DEVELOPER LIBRARY</p><h1 class="page-title">Explore topics.</h1><p>16 research directories and six practical guides. Follow the evidence, learn the mechanics and build something useful.</p><p id="learning-progress" role="status">0 of 22 topics read</p><p class="learning-note">Read markers last for this page visit.</p><h2 class="learning-group-title">Technology research</h2>'+cards(research_topics,True)+'<h2 class="learning-group-title">Developer foundations</h2>'+cards(foundation_topics,True)+'</section>'
(ROOT/'web/pages/topics.html').write_text(hub+'\n')
print('Built six foundational guides and the 22-topic dashboard/library index.')
