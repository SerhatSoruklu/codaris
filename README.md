# CODARIS

Coalition Of Developers Advancing Responsible Intelligent Systems.

This repository is the **C-first baseline**, deliberately kept small enough to understand before the product grows.

## Current architecture

```text
Browser
  -> C compiled to WebAssembly (Emscripten + semantic DOM host)
  -> future HTTPS API
Native C backend
  -> libpq
PostgreSQL
```

The v1 landing page uses a responsive HTML/CSS shell, original SVG artwork, and C/Wasm-owned demo data and interactions. There is no HTTP backend framework.

## Linux / WSL quick start

The working Linux copy is `/home/serhat/code/codaris`. The Desktop source is retained as a backup; edit the Linux copy going forward.

On Ubuntu 24.04, install the build tools once:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake emscripten libpq-dev postgresql-16 postgresql-client-16 python3
```

Run the browser client:

```bash
cd /home/serhat/code/codaris
./scripts/doctor.sh
./scripts/run-client.sh
```

Open <http://localhost:8080>. Stop with Ctrl+C. Use `PORT=8081 ./scripts/run-client.sh` for temporary testing. The server binds only to loopback and serves only `build/client`. Each launch rebuilds the C client. Ubuntu Emscripten's generated build cache is stored in ignored `.local/emscripten-cache`; the first build needs internet access.

The browser displays the CODARIS landing page with mission, sample metrics, searchable country reach, ecosystem context, and a local join-form preview. Application logic remains C/WebAssembly. The backend is a database health-check executable, not an HTTP server.

### Desktop and VS Code

Windows Desktop shortcuts:

- **Codaris - VS Code (WSL)** opens this folder in VS Code's Ubuntu environment.
- **Codaris - Run Browser** builds/serves the client in a terminal and opens the browser once ready.
- **Codaris - Browser** opens the local URL when the server is already running.

Linux application-menu entries for VS Code and Run Browser are also installed for this user. In VS Code, use **Terminal → Run Task → CODARIS: …** for build, run, database and tool checks. **Ctrl+Shift+B** builds the browser client. Run tasks use Bash in Linux/WSL; the original `.ps1` commands below remain available in Windows.

### Isolated local database

```bash
./scripts/db-local.sh start
./scripts/db-migrate.sh
./scripts/run-server.sh
./scripts/db-local.sh stop
```

This creates a project-private PostgreSQL cluster under ignored `.local/postgres`, with database `codaris` and development role `codaris_app`. It listens only on a Unix socket in a directory accessible to your Linux user; it has no TCP listener. Local trust authentication is for this private development cluster only, never production. The role owns the local cluster and is not a production least-privilege role. Data survives stop/start. The migration script skips the already-applied initial migration.

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

## 4. Build the native backend skeleton

```powershell
.\scripts\build-server.ps1
```

The CMake build uses PostgreSQL's `libpq` C client library.

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

Expected successful behavior is a PostgreSQL health check followed by a clean exit. The HTTP API layer is intentionally the next design step.

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

## Next architecture decisions

Before large implementation work, decide and document:

1. HTTP server library and request lifecycle.
2. API protocol and error model.
3. Authentication, password hashing, sessions, recovery and account verification.
4. URL normalization and LinkedIn/GitHub ownership verification.
5. Logging, metrics and audit events.
6. Test strategy and CI.
7. Linux production packaging/deployment.

Do not optimize for feature count yet. Optimize for a codebase whose boundaries remain understandable years from now.

## Landing page v1

- `web/index.html`: shared semantic document template.
- `web/pages/*.html`: dedicated homepage, mission, global reach, ecosystem, and resource content.
- `web/partials/`: shared header and footer; `web/pages.json`: route metadata.
- `web/styles.css`: responsive navy/cyan visual system, including reduced-motion support.
- `web/assets/`: original CODARIS emblem and decorative network globe; no external assets or fonts.
- `web/host.js`: browser event transport and runtime failure messaging only.
- `src/client/main.c`: C-owned filtering, metric aggregation, and local preview outcome; synchronous DOM bridge.
- `src/client/demo_data.h`: 23 illustrative country fixtures. These are invented planning examples, not researched statistics. All membership counts and activity statuses are demo data.

The join form is disabled and inert behind a centered Coming soon notice until email and application workflows are ready. Prepared validation includes field hints, native length limits, C-owned error messages and character counters. No details are collected and no registration is created. See [the frontend boundary](docs/ARCHITECTURE.md#v1-frontend-boundary) before adding submission handling.

Microsoft, LinkedIn, and Coupyn have local SVG symbols beside their names in the ecosystem section. Coupyn is a vector adaptation of the supplied reference artwork; replace it with the original production asset when available. No endorsement or active integration is claimed. Footer privacy/contact/GitHub/terms links open dedicated placeholder pages, excluded from search indexing.

## Official social channel

Per the repository owner, CODARIS uses only [X / @codarisorg](https://x.com/codarisorg) for social communications. Accounts claiming to represent CODARIS on Instagram, YouTube, Facebook, or other social platforms are fake and unaffiliated. The footer displays the official link and this notice; ecosystem platform references do not identify CODARIS social accounts.

## Static routes and production SEO

The homepage `/` keeps the hero and coalition metrics, with section 04 immediately below the metrics. Mission, Global Reach, and Ecosystem live at `/mission/`, `/global-reach/`, and `/ecosystem/`. Privacy, GitHub, Terms, and Contact have their own placeholder routes. Every page retains the shared footer. Directory routes support direct visits and refreshes on a static host without SPA fallback rules. Deploy `build/client` at the origin root, serve directory `index.html` files, and return 404 for unknown paths.

Both build scripts run `scripts/build-pages.py` using standard-library Python (Python 3.9+; `python3` on Linux, `python` on Windows). This is build tooling, not application logic. Only Home and Global Reach load the optimized C/Wasm runtime. Other pages work without JavaScript. Edit content in `web/pages/`, shared navigation/footer in `web/partials/`, and titles/descriptions/routes in `web/pages.json`. Page titles in the registry contain only the page name; the builder appends ` | CODARIS` consistently to HTML, Open Graph, Twitter, and WebPage structured-data titles. Add future editorial or service pages using the same registry and a content file. Paid services, accounts, authentication, and billing still require separate designs; no payment or account flow is implemented here.

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

The homepage displays the owner-supplied `web/assets/Codaris_Flag.png` in a framed “Proud CODARIS member” section. The original artwork is preserved, loaded lazily, and copied by the shared page builder on both Linux and Windows. Its C++ emblem is part of the supplied artwork; the implementation remains C17. The membership feature links to the existing application notice and does not indicate verified membership or open registration.

## Security and deployment

See [SECURITY.md](SECURITY.md) and the [isolated SSH deployment guide](docs/DEPLOYMENT.md). The browser build disables JavaScript string evaluation, generates a strict CSP, and includes a real 404 page. `python3 tests/check-site.py` checks the site before the allowlisted public artifact can be packaged. The GitHub page now points to [SerhatSoruklu/codaris](https://github.com/SerhatSoruklu/codaris).

The repository intentionally has no project license file. Existing third-party asset attribution remains in place.
