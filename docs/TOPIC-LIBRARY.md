# Developer topic library

The library has 22 populated topics: 16 research directories and six foundational guides. Open `/topics/` directly, or use **Explore topics** in the development member dashboard. Each card has a prominent **View topic** link; **Mark as read** is a separate, page-visit-only action on both the dashboard and public library. Reading the public library does not require a member account. Production membership routes remain gated.

## Content and provenance

The first three research directories retain their dedicated generators and documentation: [programming languages](PROGRAMMING-LANGUAGES.md), [web frameworks](WEB-FRAMEWORKS.md), and [databases](DATABASES.md).

The remaining 13 owner-supplied packs live under `data/topics/`:

| Route | Catalogue entries |
| --- | ---: |
| `/runtimes/` | 581 |
| `/package-build-tools/` | 229 |
| `/developer-environments/` | 178 |
| `/version-control/` | 118 |
| `/testing-qa/` | 277 |
| `/cloud-hosting/` | 140 |
| `/containers-infrastructure/` | 168 |
| `/cicd-automation/` | 123 |
| `/servers-networking/` | 168 |
| `/messaging-streaming/` | 133 |
| `/observability/` | 172 |
| `/apis-auth-integration/` | 226 |
| `/ai-developer-tools/` | 160 |

These packs contribute 2,673 rows; all 16 research directories contain 4,621 rows. Categories overlap, so this is not a count of globally unique technologies.

Original JSON, CSV, XLSX and README files are preserved. `manifest.json` records their SHA-256 hashes. `workbook-tables.json` preserves every non-empty worksheet cell, including supplementary tables and repeated catalogue/source sheets. Source-specific statistics, snapshot dates, definitions and metadata appear in the rendered pages. Source-pack links can provide category-level attribution rather than verification of an individual entry. Numerical claims are imported snapshots, not live or universally revalidated measurements. Missing demographics remain missing; no per-tool gender or country usage shares are inferred.

`editorial.json` adds six technical chapters per new research topic, covering mechanisms, engineering roles, failure modes, decisions, practice tasks and primary-documentation links. Editorial material is distinguished from source-pack evidence.

`foundations.json` supplies six chapters, examples and a practice project for each previously empty topic: Software Development, System Design, Python, Angular, Web Foundations and SQL/PostgreSQL. Examples teach isolated concepts and are not complete production applications. Reading examples in other languages does not change CODARIS's C17 application architecture.

## Build and edit

Run the normal client build (`./scripts/build-client.sh` or `scripts/build-client.ps1`). Both invoke `scripts/build-pages.py`, which invokes all topic generators. No new production dependency is required; Python tooling uses the standard library.

Edit research commentary in `data/topics/editorial.json` and foundational content in `data/topics/foundations.json`. Do not hand-edit generated `web/pages/<topic>.html`, `web/pages/topics.html`, `web/partials/learning-library.html` or `data/topics/library.json`. Titles, descriptions and route settings are in `web/pages.json`.

To intentionally reimport the owner's unchanged folder layout:

```bash
python3 scripts/import-topic-packs.py /mnt/c/Users/coupy/Desktop/Codaris-Topics
```

```powershell
python scripts/import-topic-packs.py 'C:\Users\coupy\Desktop\Codaris-Topics'
```

Review imported differences and checksums before accepting new research. The importer does not fetch live metrics. Existing source licenses and attribution requirements remain applicable.

## Runtime and accessibility

Pages use static semantic HTML, a documentation sidebar, native disclosure sections and keyboard-scrollable tables. Content remains readable without JavaScript. C/Wasm owns catalogue matching and the dashboard's 22-topic progress bitmask; JavaScript supplies DOM/event glue. Search enables after the runtime loads. Existing deep-link handling opens a targeted catalogue entry and its enclosing sections, and clears a conflicting search. Foundations need no Wasm runtime.

Cards stack on mobile. The primary topic link remains visually distinct from the progress toggle. Production SEO, canonical metadata and sitemap entries use the normal page registry; dashboard content is still excluded from indexing.

## Verification

```bash
python3 tests/check-topic-library.py
python3 tests/check-language-docs.py
python3 tests/check-framework-docs.py
python3 tests/check-database-docs.py
python3 tests/check-site.py
python3 tests/check-membership-build.py
```

The library check verifies original hashes, all 2,673 imported rows and values, all workbook cells and README content, six complete foundation guides, all 22 topic destinations, topic-index coverage and deterministic generation. The site check validates generated internal links, IDs, metadata and CSP. The membership check exercises development and production boundaries and restores the previous build mode.
