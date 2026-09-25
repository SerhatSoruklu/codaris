#!/usr/bin/env python3
"""Render the complete supplied research snapshot as accessible static documentation."""
import html
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'data/programming-languages'
data = json.loads((DATA / 'codaris_programming_languages_catalog.json').read_text(encoding='utf-8'))
guides = json.loads((DATA / 'guides.json').read_text(encoding='utf-8'))
notes = json.loads((DATA / 'workbook-notes.json').read_text(encoding='utf-8'))
esc = lambda value: html.escape(str(value), quote=True)
def link(url, label):
    if not url.startswith('https://'):
        raise ValueError('Expected HTTPS source')
    return f'<a href="{esc(url)}" target="_blank" rel="noopener noreferrer">{esc(label)} ↗</a>'
def table(rows, columns, caption):
    cells = []
    for row in rows:
        cells.append('<tr>' + ''.join('<td>' + esc('Not reported' if row.get(key) is None else row[key]) + '</td>' for key, label in columns) + '</tr>')
    return '<div class="research-table" tabindex="0" role="region" aria-label="' + esc(caption) + '"><table><caption>' + esc(caption) + '</caption><thead><tr>' + ''.join('<th scope="col">'+esc(label)+'</th>' for key,label in columns) + '</tr></thead><tbody>' + ''.join(cells) + '</tbody></table></div>'
parts = ['''<section class="wrap section language-library" aria-labelledby="language-title">
<div class="docs-heading"><p class="eyebrow">DEVELOPER FIELD GUIDE / 01</p><h1 class="page-title" id="language-title">Programming languages.</h1><p>Understand the tools. Read the tradeoffs. Build with intent.</p><div class="docs-stats"><span><strong>674</strong> catalogue entries</span><span><strong>15</strong> technical guides</span><span><strong>146</strong> survey geographies</span></div></div>
<div class="docs-layout"><aside class="docs-sidebar"><details open><summary>On this page</summary><nav aria-label="Programming languages documentation">
<a href="#start">Start here</a><a href="#foundations">Technical foundations</a><a href="#guides">Language field guides</a><a href="#catalogue">A–Z catalogue</a><a href="#activity">Activity &amp; usage</a><a href="#geography">Geography &amp; demographics</a><a href="#methodology">Methodology</a><a href="#sources">Sources &amp; provenance</a>
</nav><details><summary>Technical guides / 15</summary><nav aria-label="Technical language guides">''']
for i,g in enumerate(guides):
    parts.append(f'<a href="#guide-{i}">{esc(g["language"])}</a>')
parts.append('</nav></details><details><summary>Catalogue / A–Z</summary><nav class="docs-alphabet" aria-label="Catalogue letters">')
initials = list(dict.fromkeys(r['initial'] for r in data['languages']))
for i,letter in enumerate(initials):
    parts.append(f'<a href="#letter-{i}">{esc(letter)}</a>')
parts.append('''</nav></details></details><a class="text-link" href="/dashboard/">← Member workspace</a><a class="text-link" href="/topics/">All 22 topics →</a></aside><div class="docs-content">
<section id="start" class="docs-section"><p class="eyebrow">01 / ORIENTATION</p><h2>A language is a set of tradeoffs.</h2><p>Choose by the problem: deployment target, memory model, libraries, team knowledge and operational constraints. A popularity rank cannot tell you whether a language fits your system.</p><p>This library preserves the supplied research pack dated <time datetime="2026-09-23">23 September 2026</time>. Its 674-entry boundary covers notable current and historical programming languages and executable domain-specific languages—not every language ever created.</p><div class="docs-callout"><strong>Read the evidence in context.</strong><p>GitHub activity, survey use and search-interest ratings measure different things. The imported numerical snapshots have not all been independently revalidated. Technical guides are separate CODARIS editorial explanations; occupational examples are not measured demographic shares.</p></div></section>
<section id="foundations" class="docs-section"><p class="eyebrow">02 / ENGINEERING FOUNDATIONS</p><h2>What changes between languages?</h2>
<details open><summary>Type systems &amp; validation</summary><p>Static checking examines a program before execution; dynamic checks happen while it runs. Inference can reduce explicit annotations without removing static types. Neither approach validates an untrusted network message automatically. Model absent values, distinguish units, and check data at system boundaries.</p></details>
<details><summary>Execution: native code, virtual machines &amp; WebAssembly</summary><p>Ahead-of-time compilers produce code before execution; just-in-time compilers optimize during execution. Interpreters execute a representation of the program. A language can have multiple implementations. WebAssembly is a portable instruction format and compilation target, not a replacement for every host API.</p><p>CODARIS compiles C17 to WebAssembly for browser application logic. Browser glue supplies DOM and platform access. Native C handles the backend.</p></details>
<details><summary>Memory, ownership &amp; resource lifetime</summary><p>Manual management puts allocation and release under programmer control. Garbage collection reclaims unreachable memory. Ownership systems constrain who can access or release resources. Files, sockets and database handles still require explicit lifetime decisions; a memory-safe language does not eliminate resource leaks.</p></details>
<details><summary>Concurrency &amp; failure</summary><p>Threads, async tasks and actors offer different ways to organize overlapping work. Concurrency does not guarantee parallel execution. Define cancellation, deadlines, queue limits, shared-state ownership and retry behavior before scaling a service.</p></details>
<details><summary>Paradigms &amp; domain-specific languages</summary><p>Procedural code organizes steps; object-oriented code groups behavior around objects; functional code emphasizes composition and values; declarative code describes desired results. Many languages combine these. A domain-specific language narrows its vocabulary to a problem area, such as querying relational data with SQL.</p></details>
<details><summary>How to evaluate a language for a project</summary><ol><li>Write down the target platform, latency and memory limits.</li><li>Check library availability and maintenance for your actual workload.</li><li>Prototype the hardest integration and failure paths.</li><li>Measure representative performance rather than comparing toy loops.</li><li>Document build reproducibility, debugging and deployment ownership.</li></ol></details></section>
<section id="guides" class="docs-section"><p class="eyebrow">03 / TECHNICAL FIELD GUIDES</p><h2>Roles, mechanics and practical exercises.</h2><p>Fifteen starting points for deeper study. The complete research catalogue follows below; entries without an editorial guide retain their data and source links.</p>''')
for i,g in enumerate(guides):
    parts.append(f'<details class="language-guide" id="guide-{i}"><summary><strong>{esc(g["language"])}</strong><span>{esc(g["domain"])}</span></summary><div class="guide-body">')
    for key,title in [('role','Role & typical users'),('technical','Technical model'),('tradeoffs','Engineering considerations'),('exercise','Try building')]:
        parts.append(f'<h3>{esc(title)}</h3><p>{esc(g[key])}</p>')
    parts.append('<p>'+link(g['source'],'Primary documentation')+'</p></div></details>')
parts.append('''</section><section id="catalogue" class="docs-section"><p class="eyebrow">04 / COMPLETE RESEARCH INDEX</p><h2>The A–Z catalogue.</h2><p>All 674 entries from the pack. Expand a language for its available metrics and coverage notes. “Not reported” does not mean zero use.</p><div class="docs-search"><label for="catalogue-search">Find a language</label><input id="catalogue-search" type="search" maxlength="120" placeholder="Try C++, Python or Ada" disabled aria-describedby="catalogue-count"><p id="catalogue-count" role="status">674 catalogue entries · search becomes available when the runtime loads.</p></div><p id="catalogue-empty" hidden>No matching languages. Try a shorter name or clear your search.</p>''')
metrics=[('github_rank_2026_q1','GitHub rank · Q1 2026'),('github_pushers_2026_q1_approx','GitHub approximate unique pushers · Q1 2026'),('stackoverflow_rank_2025','Stack Overflow reported-use rank · 2025'),('stackoverflow_used_pct_2025','Stack Overflow reported use (%) · 2025'),('tiobe_rank_2026_09','TIOBE rank · September 2026'),('tiobe_rating_pct_2026_09','TIOBE rating (%) · September 2026')]
for i,letter in enumerate(initials):
    parts.append(f'<details class="catalogue-group" id="letter-{i}" open><summary>{esc(letter)}</summary><div class="catalogue-entries">')
    for r in data['languages']:
        if r['initial'] != letter: continue
        n=r['catalog_id']
        parts.append(f'<details class="catalogue-entry" id="language-{n}" data-catalogue-name="{esc(r["language"])}"><summary><span>{esc(r["language"])}</span><small>#{n:03d}</small></summary><div class="guide-body"><dl class="language-metrics">')
        for key,label in metrics:
            value=r[key]
            parts.append(f'<div><dt>{esc(label)}</dt><dd>{esc("Not reported" if value is None else format(value, ","))}</dd></div>')
        parts.append('</dl><p><strong>Country coverage:</strong> '+esc(r['country_breakdown_status'])+'</p><p><strong>Gender coverage:</strong> '+esc(r['gender_breakdown_status'])+'</p><p>'+link(r['catalog_source'],'Catalogue source')+' · <a href="#methodology">Metric definitions</a></p>')
        for gi,g in enumerate(guides):
            if g['language']==r['language']: parts.append(f'<a class="text-link" href="#guide-{gi}">Read the technical guide →</a>')
        parts.append('</div></details>')
    parts.append('</div></details>')
parts.append('</section><section id="activity" class="docs-section"><p class="eyebrow">05 / THREE DISTINCT MEASURES</p><h2>Activity is not market share.</h2>')
for key,title,explanation,cols in [
 ('global_github_top10_2026_q1','GitHub public activity · Q1 2026','10 rows. Approximate counts imported from the pack’s third-party visualization source. A developer may appear under several languages. These are not employment or private-code counts.', [('rank','Rank'),('language','Language'),('approx_unique_pushers','Approx. unique pushers')]),
 ('stackoverflow_used_2025','Stack Overflow reported use · 2025','21 rows. Multiple selections are possible, so percentages overlap. The pack cites 31,771 answers to the language question. HTML/CSS remains here because the survey includes markup; it is not added to the core executable-language catalogue.', [('rank','Rank'),('language','Language / category'),('used_pct','Respondents using (%)')]),
 ('tiobe_2026_09','TIOBE popularity · September 2026','20 rows imported from the supplied pack’s secondary-source snapshot. This is a search-interest/popularity rating, not measured developer usage. It is not a recommendation or a combined score.', [('rank','Rank'),('language','Language'),('rating_pct','Rating (%)')])]:
    parts.append('<details open><summary>'+title+'</summary><p>'+esc(explanation)+'</p>'+table(data[key],cols,title)+'<p><a href="#sources">Snapshot sources and attribution ↓</a></p></details>')
parts.append('''</section><section id="geography" class="docs-section"><p class="eyebrow">06 / PEOPLE &amp; PLACE</p><h2>Who is represented?</h2><p>A country table describes the survey sample—not a count of all developers living there, and not a per-language country breakdown. Missing economies and non-response affect comparisons.</p><details><summary>All 146 survey country / region rows</summary>'''+table(data['stackoverflow_country_respondents_2025'],[('country','Country / region'),('responses','Responses'),('percent','Share of respondents (%)')],'Stack Overflow 2025 · geography from supplied pack')+'</details><details><summary>Bangladesh · Q1 2026 language activity example</summary><p>Five rows retained from the pack’s secondary report. One economy cannot describe worldwide language demographics. Counts can overlap between languages.</p>'+table(data['country_language_examples'],[('economy','Economy'),('language','Language'),('unique_pushers','Unique pushers'),('year','Year'),('quarter','Quarter')],'Bangladesh · source-pack activity example')+'</details><h3>Gender: no inferred percentages</h3><p>The pack contains no verified per-language gender breakdown. Language choice is not evidence of anyone’s gender. A future analysis would need respondent-level data, explicit sample sizes, survey year, uncertainty and the source’s non-binary, undisclosed and missing categories.</p>')
parts.append('</section><section id="methodology" class="docs-section"><p class="eyebrow">07 / RESEARCH NOTES</p><h2>Scope, definitions and limitations.</h2>')
for title,rows in notes.items():
    parts.append('<details><summary>'+esc(title)+' · workbook notes</summary>')
    for row in rows:
        parts.append('<p><strong>'+esc(row[0])+'</strong>'+(' — '+' · '.join(esc(v) for v in row[1:]) if len(row)>1 else '')+'</p>')
    parts.append('</details>')
parts.append('</section><section id="sources" class="docs-section"><p class="eyebrow">08 / PROVENANCE</p><h2>Follow the evidence.</h2><p>All 11 source records are retained below. Original JSON, CSVs, workbook and README are preserved in the repository. Imported metrics remain attributed snapshots, not live counters. Catalogue attribution: Wikipedia’s List of programming languages; the pack describes its normalized factual index and CC BY-SA source terms. GitHub Innovation Graph data is CC0-1.0. Other source terms remain applicable.</p>')
for r in data['sources']:
    parts.append('<article class="research-source"><h3>'+esc(r['dataset'])+'</h3><p>'+link(r['url'],r['source'])+'</p><p>'+esc(r['notes'])+'</p></article>')
parts.append('</section><a class="text-link" href="#language-title">Back to top ↑</a></div></div></section>')
output='\n'.join(parts)+'\n'
(ROOT / 'web/pages/programming-languages.html').write_text(output,encoding='utf-8')
print(f'Built language documentation: {len(data["languages"])} entries, {len(guides)} guides, all research tables.')
