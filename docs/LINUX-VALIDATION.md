# Linux baseline validation — 2026-09-22

Historical SDL baseline results. For the current landing page, see [V1 validation](V1-VALIDATION.md).

Environment: Ubuntu 24.04 under WSL, GCC 13.3, CMake 3.28, Emscripten 3.1.6, PostgreSQL/libpq 16.15, Chromium via Playwright.

Observed results:

- Native C17 backend builds with the configured warning flags.
- C/SDL2 WebAssembly client builds using a writable project-local Emscripten cache.
- Private PostgreSQL cluster starts/stops; initial migration succeeds and a second invocation skips it.
- Backend connects successfully as `codaris_app` to `codaris`, including after database restart.
- Chromium loads `codaris.wasm` with HTTP 200, initializes WebGL, and advances the C render loop without aborting.
- Reload works. Rendering continues at desktop (1440×900) and narrow (390×844) viewports.
- Final browser console has no errors. Packaged SDL/Emscripten emits a nonfatal main-loop timing warning at initialization; the first browser session also reported software GPU performance warnings.
- Desktop Run Browser shortcut actually starts the Linux server on localhost:8080.
- Bash scripts pass syntax checks.

The existing canvas remains fixed at 1280×720 and overflows narrow viewports. Responsive design is not implemented in this baseline. There are no interactive product flows or HTTP backend routes to test yet.

Screenshot: `output/playwright/codaris-linux.png` (ignored local artifact).

Temporary browser server on port 8081 and project-private database were stopped after validation. The desktop-launched browser server on port 8080 is left running for use; Ctrl+C in its terminal stops it.
