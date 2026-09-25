# Programming languages library

`/programming-languages/` is a public, static educational topic linked from the member dashboard's learning panel. The page does not require membership authentication. All source-pack content is public research, not member data.

## Data and provenance

The supplied Desktop folder was named `CODARIS_programming_languages_research_pack`. Its five original files are preserved under `data/programming-languages/`. JSON provides all 674 catalogue entries, 10 GitHub rows, 21 Stack Overflow usage rows, 20 TIOBE rows, 146 respondent-country rows, five Bangladesh examples and 11 source records. `workbook-notes.json` additionally preserves the workbook's dashboard, gender and methodology notes; other sheets duplicate the structured data. Original files are source artifacts, not copied into the public deployment bundle.

Snapshots retain their dates, attribution, approximations and nulls. Imported numerical values are not all independently revalidated. No composite score or gender estimate is created. Survey geography is sample composition, not national developer population. Source terms and caveats remain visible in the page.

`guides.json` contains 15 separately authored, concise technical guides with primary documentation links. These describe typical engineering roles, not measured demographic distributions. The remaining catalogue entries do not claim to have researched technical profiles.

## Build and interaction

The shared Python builder invokes `scripts/build-language-docs.py` on both Bash and PowerShell builds. It escapes content and generates `web/pages/programming-languages.html` deterministically from repository data. Edit the data or generator, not generated markup. No new package dependency is required.

Native HTML details provide accessible open/close sections. The complete content remains readable without Wasm. `src/client/languages.h` owns catalogue substring matching; the browser bridges only transport input and display results. Search is ASCII case-insensitive, which covers the source language names, and literal for non-ASCII bytes. Search queries stay local and are never persisted. Browser glue reveals deep-linked disclosure sections and clears a conflicting catalogue filter.

Run `python3 tests/check-language-docs.py`, `python3 tests/check-site.py`, and the existing membership boundary checks after building.
