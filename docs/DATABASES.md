# Databases and data platforms

The public `/databases/` topic is linked from the member learning panel. It shares the documentation layout, native disclosure navigation and C/Wasm catalogue search. All content remains readable without JavaScript or a member account.

## Research coverage

All seven original files from `CODARIS_databases_research_pack` are preserved under `data/databases/`. JSON provides 643 catalogue entries and 21 source records. `workbook-tables.json` retains the overview, 16 developer-use rows, DB-Engines top 20, nine model-ranking rows, 24 developer-platform profiles, 146 survey-geography rows and methodology, including workbook-only labels and notes.

Snapshots are dated 23 September 2026 or the survey year shown. Imported numbers are not all independently revalidated. DB-Engines scores are popularity indices, not percentages. Survey usage is multi-select; geography describes the respondent sample, not database adoption by country. No gender statistics are inferred. Product type, data model, deployment and provider remain distinct fields. Dated platform descriptions do not establish current offerings.

Twelve editorial guides in `guides.json` explain technical models, typical users, tradeoffs and exercises, linking to primary documentation. These role examples are not demographic measurements. Other catalogue entries retain source data without claiming a full technical profile.

## Maintenance

`scripts/build-database-docs.py` produces escaped static markup at `web/pages/databases.html`; the shared builder invokes it for Bash and PowerShell builds. Edit the source JSON or generator rather than generated HTML. Original research files remain repository source artifacts and are not copied into deployment output. No additional dependency or build-time network request is introduced.

The existing filter in `src/client/languages.h` matches database names, primary/secondary models, type, deployment and provider. Search stays local. Browser glue reveals linked disclosures and resets conflicting filters. The dashboard adds the ninth topic and counts its local explored state.

Validation: `python3 tests/check-database-docs.py`, `python3 tests/check-site.py`, membership build-boundary tests, and browser checks of search, deep links, mobile layout, native disclosures without JavaScript, and dashboard integration.
