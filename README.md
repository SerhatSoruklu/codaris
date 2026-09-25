# CODARIS

Coalition Of Developers Advancing Responsible Intelligent Systems.

C17 application code, WebAssembly in the browser, and a native PostgreSQL-backed account API. See the [account design](docs/ACCOUNT-SERVICE.md) and [release runbook](docs/RELEASE.md) for current setup and release requirements.

## Current architecture

```text
Browser
  -> C compiled to WebAssembly (Emscripten + semantic DOM host)
  -> same-origin HTTPS /api/
Native C backend
  -> libpq
PostgreSQL
```

The v1 landing page uses a responsive HTML/CSS shell, original SVG artwork, and C/Wasm account interactions. The native API uses libmicrohttpd, libpq, libsodium, libcurl and json-c.

## Environment selection

Real dev/prod configuration lives in four Git-ignored `.env` files under `config/`. Use `python3 scripts/project.py dev build` or `python3 scripts/project.py prod build`; run commands select the corresponding environment the same way. On Windows use `python`. A fresh checkout creates missing files with `python scripts/init-env.py`. See [configuration instructions](config/README.md). Production and development backend binaries are separate, and backend credentials never enter frontend builds.

## Linux / WSL quick start

The working Linux copy is `/home/serhat/code/codaris`. The Desktop source is retained as a backup; edit the Linux copy going forward.

On Ubuntu 24.04, install the build tools once:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake emscripten libpq-dev libmicrohttpd-dev libsodium-dev libcurl4-openssl-dev libjson-c-dev postgresql-16 postgresql-client-16 python3
```

Start the full local app with one command:

```bash
cd /home/serhat/code/codaris
./scripts/dev.sh
```

This starts the project-private PostgreSQL database, applies pending migrations, builds and starts the C account API, builds and serves the browser client, and opens the browser when available. Keep this terminal running while developing; press Ctrl+C to stop the API and browser server. The database remains available for the next run. Check <http://127.0.0.1:8081/api/health>; registration needs `"message":"Ready"`. New registration queues a verification email; run `python3 scripts/project.py dev mail` when you want the configured mail worker to deliver queued messages.

### Desktop and VS Code

Windows Desktop shortcuts:

- **Codaris - VS Code (WSL)** opens this folder in VS Code's Ubuntu environment.
- **Codaris - Run Browser** starts the local database, account API and browser client, then opens the browser.
- **Codaris - Browser** opens the local URL when the server is already running.

Linux application-menu entries for VS Code and Run Browser are also installed for this user. In VS Code, use **Terminal → Run Task → CODARIS: …** for build, run, database and tool checks. **Ctrl+Shift+B** builds the browser client. Run tasks use Bash in Linux/WSL; the original `.ps1` commands below remain available in Windows.

### Isolated local database

```bash
./scripts/db-local.sh start
./scripts/db-migrate.sh
./scripts/run-server.sh
./scripts/db-local.sh stop
```

This creates a project-private PostgreSQL cluster under ignored `.local/postgres`, with database `codaris` and development role `codaris_app`. It listens only on a Unix socket in a directory accessible to your Linux user; it has no TCP listener. Local trust authentication is for this private development cluster only, never production. The role owns the local cluster and is not a production least-privilege role. Data survives stop/start. The migration runner applies sequential migrations and skips recorded versions.

To target another database, explicitly provide `PGHOST`, `PGPORT`, `PGDATABASE`, `PGUSER` and a libpq password file. The backend also supports `CODARIS_DATABASE_URL`; do not store credentials in source control. No existing external database is needed for browser testing.

## Initial local target

The current Windows development setup assumes:

- Windows 11
- PowerShell
- VS Code
- Git
- Emscripten SDK at `C:\emsdk`
- PostgreSQL 18 at `C:\Program Files\PostgreSQL\18`
- CMake 3.27+
- Microsoft C/C++ build tools (MSVC + Windows SDK)

Paths are defaults, not architectural requirements. Override `PostgreSQL_ROOT` or `EMSDK` when needed.

## VS Code extensions

This repo recommends:

- OpenAI Codex (`openai.chatgpt`)
- Microsoft C/C++ (`ms-vscode.cpptools`)
- Microsoft CMake Tools (`ms-vscode.cmake-tools`)
- Microsoft PostgreSQL (`ms-ossdata.vscode-pgsql`)
- EditorConfig (`editorconfig.editorconfig`)

VS Code can offer to install these automatically from `.vscode/extensions.json`.

## 1. Check your machine

```powershell
.\scripts\doctor.ps1
```

## 2. Apply the initial database migration

The database and role are expected to already exist:

- database: `codaris`
- role: `codaris_app`

Run:

```powershell
.\scripts\db-migrate.ps1
```

`psql` will request the password unless your libpq/PostgreSQL credential mechanism already supplies it.

## 3. Build the browser client

```powershell
.\scripts\build-client.ps1
```

The script compiles `src/client/main.c` into WebAssembly and places browser output in `build/client`.

Run it in Chrome:

```powershell
.\scripts\run-client.ps1
```

The client renders an accessible multi-page experience. Search and form preview require the compiled Wasm module.

## 4. Build the native account API

```powershell
.\scripts\build-server.ps1
```

The CMake build uses the native dependencies listed in [ACCOUNT-SERVICE.md](docs/ACCOUNT-SERVICE.md). On Windows supply the vcpkg CMake toolchain.

For a development shell, one option is to provide libpq connection values through environment variables:

```powershell
$env:PGHOST = "localhost"
$env:PGPORT = "5432"
$env:PGDATABASE = "codaris"
$env:PGUSER = "codaris_app"
$env:PGPASSWORD = "<development-password>"
```

Do not commit the password. Prefer a proper local secret/password mechanism as the project matures.

Run:

```powershell
.\scripts\run-server.ps1
```

The API remains running on loopback. Provide backend configuration and a mail encryption key before launch; `/api/health` checks database readiness. See [RELEASE.md](docs/RELEASE.md).

## Repository layout

```text
.
├── AGENTS.md
├── CMakeLists.txt
├── include/codaris/
├── src/
│   ├── client/
│   └── server/
├── db/migrations/
├── docs/
├── scripts/
└── web/
```

## Account architecture

The account lifecycle, dependencies, transactions, verification, SMTP queue and security boundaries are documented in [ACCOUNT-SERVICE.md](docs/ACCOUNT-SERVICE.md). Production infrastructure and delivered-email checks remain separate from local integration testing.

## Landing page v1

- `web/index.html`: shared semantic document template.
- `web/pages/*.html`: dedicated homepage, mission, global reach, ecosystem, and resource content.
- `web/partials/`: shared header and footer; `web/pages.json`: route metadata.
- `web/styles.css`: responsive navy/cyan visual system, including reduced-motion support.
- `web/assets/`: original CODARIS emblem and decorative network globe; no external assets or fonts.
- `web/host.js`: browser event transport and runtime failure messaging only.
- `src/client/main.c`: C-owned account interactions and browser transport.
- `src/client/demo_data.h`: legacy country fixtures; only country labels remain in the registration selector. Published membership totals come from PostgreSQL.

The join form activates after Wasm loads and submits to the account API. Accounts require email verification; settings and learning progress persist in PostgreSQL.

Microsoft, LinkedIn, and Coupyn have local SVG symbols beside their names in the ecosystem section. Coupyn is a vector adaptation of the supplied reference artwork; replace it with the original production asset when available. No endorsement or active integration is claimed. Footer links open their dedicated pages; privacy and terms include the supplied operator details.

## Official social channel

Per the repository owner, CODARIS uses only [X / @codarisorg](https://x.com/codarisorg) for social communications. Accounts claiming to represent CODARIS on Instagram, YouTube, Facebook, or other social platforms are fake and unaffiliated. The footer displays the official link and this notice; ecosystem platform references do not identify CODARIS social accounts.

## Static routes and production SEO

The homepage `/` keeps the hero and coalition metrics, with the member flag and technology sections below the metrics. Section 04 / Join the Coalition now lives at `/join/`. Mission, Global Reach, and Ecosystem live at `/mission/`, `/global-reach/`, and `/ecosystem/`. Privacy, GitHub, Terms, and Contact have dedicated routes. Every page retains the shared footer. Directory routes support direct visits and refreshes on a static host without SPA fallback rules. Deploy `build/client` at the origin root, serve directory `index.html` files, and return 404 for unknown paths.

Both build scripts run `scripts/build-pages.py` using standard-library Python (Python 3.9+; `python3` on Linux, `python` on Windows). This is build tooling, not application logic. Interactive account, globe and catalogue pages load the optimized C/Wasm runtime. Editorial pages remain readable without JavaScript. Edit content in `web/pages/`, shared navigation/footer in `web/partials/`, and titles/descriptions/routes in `web/pages.json`. Page titles in the registry contain only the page name; the builder appends ` | CODARIS` consistently to HTML, Open Graph, Twitter, and WebPage structured-data titles. Add future editorial or service pages using the same registry and a content file. Account services are implemented; payments and billing are not.

The configured production origin is `https://codaris.org` in `web/site.json`. An optional `CODARIS_SITE_URL` overrides it. Local builds use `noindex`; production builds enable indexing for substantive pages:

```bash
CODARIS_PRODUCTION=1 ./scripts/build-client.sh
```

```powershell
$env:CODARIS_PRODUCTION = "1"
.\scripts\build-client.ps1
Remove-Item Env:CODARIS_PRODUCTION
```

The build emits canonical URLs, social metadata, WebSite/Organization/WebPage and breadcrumb structured data, `robots.txt`, and a sitemap containing the substantive pages (including the published GitHub resource). Placeholder pages remain `noindex, follow`. After deploying, submit `https://codaris.org/sitemap.xml` to Search Console. Titles, headings, and crawlable links support discovery; Google decides whether to display sitelinks. Enable Brotli/gzip and appropriate asset caching at the production host, serve `.wasm` as `application/wasm`, and redirect alternate hosts/HTTP to the canonical HTTPS origin. Compression and hosting headers are not provided by the local preview server.

## Homepage membership feature

The homepage displays the owner-supplied `web/assets/Codaris_Flag.png` in a framed “Proud CODARIS member” section. The original artwork is preserved, loaded lazily, and copied by the shared page builder on both Linux and Windows. Its C++ emblem is part of the supplied artwork; the implementation remains C17. The membership feature links to registration and does not certify the visitor’s membership.

## Security and deployment

See [SECURITY.md](SECURITY.md) and the [isolated SSH deployment guide](docs/DEPLOYMENT.md). The browser build disables JavaScript string evaluation, generates a strict CSP, and includes a real 404 page. `python3 tests/check-site.py` checks the site before the allowlisted public artifact can be packaged. The GitHub page now points to [SerhatSoruklu/codaris](https://github.com/SerhatSoruklu/codaris).

The repository intentionally has no project license file. Existing third-party asset attribution remains in place.

The home hero uses a full-width background, confined to hero height, with a local SVG code-rain tile in the cyan/navy palette. Two CSS transform layers provide motion (one on small screens), with no animation library or frame-by-frame JavaScript. Browser visibility events pause it outside the viewport or in a hidden tab; “Pause background” freezes it manually. Reduced-motion preferences keep the decoration static.

Mission, Global Reach, and Ecosystem share photographic-style concept-image heroes, fixed image framing, localized screen/server activity, a pause control, and reduced-motion support. Current images are 1672 × 941 (not native 4K), exported at WebP quality 95, with actual file sizes documented by the asset files. The shared page builder copies them on both platforms and the deployment packager accepts them. Hero presentation requires no JavaScript; Global Reach shows verified membership aggregates. See [artwork prompts and resolution notes](docs/PHOTOGRAPHIC-ART.md).

## Membership and email

Registration, login, recovery, editable profile and security settings use the native account API in both build modes. Email changes immediately clear verification; the original joining reason is read-only. Profile pictures store fixed 100×100 pixel data, not original files. Topic progress persists across visits. Staff tools and chat are explicitly unavailable.

Production bundles require `CODARIS_PRODUCTION=1`. Run `python3 tests/check-membership-build.py` for both UI build modes and `python3 tests/check-accounts.py` against an isolated local test database. Google Workspace SMTP matches the Serhat app’s `admin@coupyn.com` setup. See [RELEASE.md](docs/RELEASE.md) for private configuration and deployment. SMTP authentication alone is not proof of delivered mail or a completed release.

The [programming languages library](docs/PROGRAMMING-LANGUAGES.md) adds `/programming-languages/` to the learning workspace: all 674 supplied catalogue entries, research snapshots and 15 technical guides, with native disclosure navigation and C/Wasm search.
The [web frameworks and technologies library](docs/WEB-FRAMEWORKS.md) adds `/web-frameworks/`: 631 source-pack entries, 12 technical guides, all workbook research tables and shared C/Wasm catalogue search.
The [databases and data platforms library](docs/DATABASES.md) adds `/databases/`: 643 catalogue entries, 12 technical guides, 24 platform profiles and all supplied usage, popularity and demographic-context tables.

The [complete developer topic library](docs/TOPIC-LIBRARY.md) now provides 22 populated topics at `/topics/` and in the dashboard: 16 research directories with 4,621 catalogue rows (including overlaps), plus six foundational guides with examples and practice projects. The 13 additional research packs retain all supplied tables and source notes, with six editorial technical chapters each. Dashboard cards have clear **View topic** links and separate persistent **Mark as read** controls in the signed-in dashboard. Run `python3 tests/check-topic-library.py` to check source integrity and complete rendering.
