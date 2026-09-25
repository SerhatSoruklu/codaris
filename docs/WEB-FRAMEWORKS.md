# Web frameworks and technologies

The public `/web-frameworks/` topic is linked from the member learning panel. It uses the existing documentation layout and native disclosure navigation; reading requires no authentication or JavaScript. The catalogue covers frameworks, libraries, runtimes, CMS tools and static-site technologies, preserving the research pack's type labels rather than treating every entry as a framework.

## Sources

All six original files from `CODARIS_web_frameworks_research_pack` are preserved in `data/web-frameworks/`. JSON supplies the 631-entry catalogue and 21 source records. `workbook-tables.json` preserves workbook-only details and tables: overview, 26 survey-use rows, 20 JS-sentiment rows, three website-detection rows, catalogue breakdown, 146 geography rows and methodology. Original data files are source artifacts and are not copied to the public deployment bundle.

The pack is dated 23 September 2026. Imported numerical values are attributed snapshots, not all independently revalidated. Survey use, satisfaction and website detection have different denominators. Geography describes respondents, not framework use by country. No gender values are invented. Some imported classifications combine types; the 12 separately authored technical guides clarify their covered tools with links to primary documentation. These guides describe engineering roles, not demographic estimates.

## Build and interaction

`scripts/build-framework-docs.py` generates `web/pages/web-frameworks.html` with escaped content. The shared page builder invokes it on both Bash and PowerShell paths; edit the generator or JSON rather than generated markup. There are no additional dependencies or network requests during builds.

The shared C catalogue filter in `src/client/languages.h` is used by both documentation topics. Framework search includes name, category, layer, type, language and ecosystem; programming-language search keeps matching language names. Browser glue handles only input transport, rendering and opening deep-linked disclosures. Queries are local and not persisted. Native disclosures remain usable when the runtime fails.

Validation: `python3 tests/check-framework-docs.py`, `python3 tests/check-language-docs.py`, the site checker, membership production-boundary checks, and browser checks for both catalogues and the eight-topic dashboard counter.
