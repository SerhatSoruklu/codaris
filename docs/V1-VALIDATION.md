# V1 landing-page validation — 2026-09-22

Executed on Linux/WSL with packaged Emscripten and cached Playwright Chromium.

- `./scripts/build-client.sh`: successful C17/Wasm build, no compiler warnings.
- `bash -n scripts/build-client.sh scripts/run-client.sh`: passed.
- `node --check web/host.js`: passed.
- Chromium loaded the real compiled module and rendered all 23 rows and C-derived metrics.
- Case-insensitive country search, no-results state, literal HTML-like input, and clearing search passed.
- Empty form submission was blocked by native validation. A valid sample submission displayed the local-only result, cleared inputs, focused feedback, and made zero network requests.
- No document overflow at 1440, 768, 390, and 320 pixel widths after fixing the narrow hero layout. The wide table intentionally scrolls within its own region.
- Keyboard skip-link focus and reduced-motion scroll behavior passed.
- Simulated Wasm download failure displayed the unavailable message and kept search/form disabled; unblocking and reloading restored all country rows. This check exposed and led to fixing an older Emscripten fetch-failure path that otherwise remained stuck loading.
- No page exceptions were observed on the successful load/recovery. Console errors during intentional download blocking are expected.
- Desktop and mobile screenshots were visually inspected. Local ignored artifacts: `output/playwright/codaris-v1-desktop.png`, `codaris-v1-full.png`, and `codaris-v1-mobile.png`.

PowerShell build/run helpers were updated consistently but not executed: PowerShell is unavailable in this environment. Backend/database files were unchanged; no backend or migration checks were necessary. The workspace has no `.git` directory, so Git diff/status verification is unavailable.

The temporary preview on port 8082 was stopped after verification. Use `./scripts/run-client.sh` to rebuild and serve on port 8080.

## Geographic globe update

C17/Wasm rebuild passed. Chromium verified no document overflow at widths 320, 390, 768, 1024 and 1440 after correcting SVG containment. Pause checkbox sets route animations to paused; reduced-motion disables them. Desktop/mobile globe screenshots were captured in `output/playwright/globe-desktop.png` and `globe-mobile.png`. The fixed projection keeps the red USA and blue custom island region visible.

## Interactive globe update

C17/Wasm build passed without warnings. Chromium mouse drag changed projected country geometry; reset and keyboard Home restored identical projected paths. Arrow-key navigation passed. No document overflow at 320, 390, 768, 1024, or 1440px. Coordinate and rotated-view screenshots were inspected. Rotation remains entirely C-owned, with DOM event transport in the host. PowerShell execution remains untested.
