# CODARIS

**Coalition Of Developers Advancing Responsible Intelligent Systems**

**Build. Verify. Advance. Trust is engineered.**

CODARIS is an early-stage, developer-focused coalition and a growing technical platform. Its purpose is to bring developers together around the engineering of responsible intelligent systems: practical work, careful verification, security, safety, interoperability and clear evidence about what systems do.

The intended progression is:

> Coalition → meaningful membership → real activity → governance → ecosystem → identify problems → build mechanisms that solve them

That progression is a direction, not a claim that every stage or institution is already in place. CODARIS should earn its structure by doing useful technical work. Branding, membership counts and empty governance structures are not substitutes for work that others can inspect and use.

## What CODARIS is

CODARIS is an independent developer coalition operated by Serhat Soruklu. It is intended for developers and other technical contributors who want to work on responsible intelligent systems and the software, infrastructure and practices around them.

The public site currently provides information about the mission, membership approach, organizational participation plans, legal terms and privacy, plus a substantial developer learning library. The repository contains the C/WebAssembly web client, native C account and contact API, PostgreSQL schema, build tooling and deployment documentation.

## Why CODARIS exists

Intelligent systems affect people through more than model behavior. Their surrounding software, data, interfaces, infrastructure, integrations and operational decisions also matter. CODARIS exists to create a place where developers can examine those systems together and turn concerns into specific engineering questions and useful work.

The coalition’s stated principles are progress, safety, developer leadership, a global perspective, accountability and transparency about the difference between current capabilities and future plans. Technical credibility should come from documented reasoning, reproducible evidence and review, not from broad claims of authority.

## Why join

Joining is for people who want to contribute expertise, questions and effort to a developer-led coalition. The current membership flow is intentionally modest: it establishes an account and membership after email confirmation. It does not promise employment, certification, professional accreditation, a public directory listing, access to an active working group, or influence over a formal governance process.

When the account service is deployed and available, members can maintain their account, control whether their membership credential is publicly verifiable, and keep progress through the learning topics. The public learning library itself can be read without an account. Membership is not required to use its guides or catalogues.

## Who CODARIS is for

CODARIS is for developers and technically engaged people interested in building, evaluating or operating responsible intelligent systems. The site also invites technical, research, open-source and organizational conversations. The proposed organizational participation framework is still in development; a conversation or platform reference does not mean an organization is participating, sponsoring or endorsing CODARIS.

## Current platform

The public site is marked Beta V1. It currently includes:

- Mission, global perspective, ecosystem and organizational participation pages. The organizational framework, directories, working groups and safeguards described there are proposals or marked as coming soon, not active programs.
- A public developer library: 16 research catalogues and six foundational guides, with source notes, technical chapters and exercises. Catalogue material includes dated snapshots and source limitations; it should not be read as live market data or a ranking.
- An implemented account service in the codebase: registration, email verification, login, password recovery and change, editable profile settings, membership credentials, optional public credential verification, and saved topic progress.
- A member dashboard in the codebase for profile and security settings, credential controls, and topic progress. Community/chat is a preview, not a live discussion service.
- A contact form implementation, with messages queued by the native service for delivery.

**Deployment status matters:** the account and contact workflows exist in the repository and can be run locally, but the repository does not establish whether the public API, database, mail delivery and operational checks are currently available on the live host. The release runbook requires those checks before public registration is advertised. The production client build intentionally disables the Join form; development builds can expose an interactive preview. A repository feature is not evidence that the corresponding production service is live.

Membership activates automatically after email verification; there is no staff application-review or approval workflow. Email confirmation proves access to a mailbox, not a person’s identity, qualifications, country or ownership of external profile links. A member may choose to enable a credential verification link. Anyone with that link can see the limited fields disclosed in the terms and privacy notice. CODARIS does not currently provide a browsable member directory.

The ecosystem page names Microsoft, LinkedIn and Coupyn as platform or operational context. It explicitly does not claim partnerships, endorsement or an active Coupyn integration. The organization participation page currently has no organization, representative, supporter or working-group listings. CODARIS has no paid membership feature.

## Direction: what comes next

The next meaningful step is real activity: find problems worth solving, bring people together around clear technical questions, and produce work that can be reviewed. Depending on the problem and what contributors decide to pursue, useful outputs could include:

- threat models and security research;
- specifications and interoperability work;
- reference implementations and tests;
- red-team findings and evaluations; and
- deployment guidance with documented assumptions and limitations.

These are examples of possible future work, not a committed release plan or claims about existing CODARIS publications. Governance and organizational participation should grow from sustained contributions, transparent process and demonstrated need. No active standards program, research group, organizational membership scheme or formal governance body is represented as operating today.

## Technical stack

The project keeps application logic in C. The browser application is C17 compiled to WebAssembly with Emscripten; semantic HTML and CSS provide document structure and presentation, while a small JavaScript host forwards browser events. Static routes are generated at build time by standard-library Python. The native backend is C17, uses libmicrohttpd for HTTP, libpq for PostgreSQL, libsodium for password hashing and token protection, libcurl for SMTP, json-c for JSON, and vendored libHaru/Nayuki QR code generation for membership credentials.

```text
Browser: semantic HTML/CSS + C17 → WebAssembly
                    │ same-origin HTTPS /api/
                    ▼
Native C API ── libpq ── PostgreSQL
       └── SMTP delivery worker
```

PostgreSQL is authoritative for account and progress data. Database credentials and mail secrets are backend-only; they must never enter the browser or Wasm build. See [architecture](docs/ARCHITECTURE.md), [account service design](docs/ACCOUNT-SERVICE.md), and [security notes](SECURITY.md).

## Local development

### Prerequisites

Linux/WSL development uses Bash, Python 3, CMake 3.27+, a C17 compiler, Emscripten, PostgreSQL/libpq, libmicrohttpd, libsodium, libcurl and json-c. On Ubuntu 24.04, install the system packages with:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake emscripten libpq-dev libmicrohttpd-dev libsodium-dev libcurl4-openssl-dev libjson-c-dev postgresql-16 postgresql-client-16 python3
```

Windows 11 development is supported with PowerShell, Git, Python, CMake 3.27+, Emscripten, MSVC and the Windows SDK, PostgreSQL, and native dependencies installed through vcpkg. See [account service dependencies](docs/ACCOUNT-SERVICE.md) and the scripts for toolchain details. Defaults such as `C:\emsdk` and the PostgreSQL install path are configurable.

### Start the development app

From the repository root:

```bash
./scripts/dev.sh
```

The launcher checks the selected development configuration, starts the project-private PostgreSQL cluster, applies pending migrations, builds and starts the native API, builds and serves the client, and runs the local mail worker when configured. The default frontend is `http://127.0.0.1:8081`; the API defaults to `127.0.0.1:8080`. Check `http://127.0.0.1:8081/api/health` for local service readiness. Press Ctrl+C to stop processes started by the launcher; the local database data remains available for the next run.

The scripts use a local SMTP capture service by default, so development mail is not sent to real recipients. To run the parts separately, use:

```bash
python3 scripts/project.py dev check-env
python3 scripts/project.py dev migrate
python3 scripts/project.py dev build
python3 scripts/project.py dev run-server
# In separate terminals:
python3 scripts/project.py dev run-client
python3 scripts/project.py dev mail
```

Use `python` instead of `python3` on Windows. `dev`/`prod` and `development`/`production` are accepted mode names. The `build` action builds both client and server; use `build-client` or `build-server` for one component.

### Build and run components directly

```bash
./scripts/build-client.sh
./scripts/run-client.sh
./scripts/build-server.sh
./scripts/run-server.sh
```

The client output is written to `build/client`. For a production-indexed static site, use `python3 scripts/project.py prod build-client`; the package workflow is documented in [deployment](docs/DEPLOYMENT.md). The native server build uses CMake. On Windows, corresponding `.ps1` scripts are provided. For a manual local database lifecycle, use `scripts/db-local.sh start`, `scripts/db-migrate.sh`, and `scripts/db-local.sh stop`; the private Linux cluster lives under ignored `.local/postgres` and listens on a local Unix socket. The development role owns that local cluster and is not a production least-privilege role.

## Configuration

`python3 scripts/init-env.py` creates missing local configuration files without overwriting existing ones. The four Git-ignored files are:

- `config/frontend.development.env`
- `config/frontend.production.env`
- `config/backend.development.env`
- `config/backend.production.env`

Select the environment explicitly with `python3 scripts/project.py dev ACTION` or `prod ACTION`. The selected file takes precedence over inherited shell settings. Frontend configuration rejects backend-only values. Frontend API calls use same-origin `/api/`; `CODARIS_SITE_URL` configures the canonical public site URL, not the API destination. See [configuration documentation](config/README.md) for supported variables and platform-specific defaults.

For a separate database, configure PostgreSQL using `PGHOST`, `PGPORT`, `PGDATABASE`, `PGUSER`, and a protected libpq password file, or use backend-only `CODARIS_DATABASE_URL`. Do not commit credentials. `CODARIS_MAIL_KEY` is a private, environment-specific 32-byte key used to protect queued action tokens. Never put database credentials, SMTP passwords or this key in frontend configuration or generated browser output.

## Project structure

```text
include/codaris/       Public C service/database interfaces
src/client/            C17 browser application compiled to WebAssembly
src/server/            Native C API, PostgreSQL access, mail and credentials
src/shared/            Shared C definitions
web/                   Static page sources, assets, styles and browser host glue
web/pages.json         Static route metadata and indexability policy
data/                  Source catalogues and topic content
db/migrations/         Sequential PostgreSQL schema migrations
scripts/               Cross-platform build, run, import and deployment tooling
deploy/                Nginx/systemd examples and deployment configuration
docs/                  Architecture, account, operations and content documentation
tests/                 Site, data-generation and account checks
```

Static pages are authored in `web/pages/` and assembled from `web/index.html`, shared partials and `web/pages.json` by `scripts/build-pages.py`. Generated topic pages and catalogues have source data and editing guidance in their relevant `docs/` files. Do not edit generated build output as the source of truth.

## Current status

CODARIS is an early-stage Beta V1 project. The public site and developer learning library are available as static content. Account, credential, contact and mail functionality is implemented in the repository and has local development/test workflows. The production client build disables the Join form; development builds can expose the preview form. The repository does not establish whether the public account API and its production operations are currently available. Verify the API, database, mail delivery and operational checks before treating registration as live or enabling the form.

The repository includes production build and deployment tooling, Nginx and systemd examples, an allowlisted static-site package, and a release runbook. These files describe how to deploy; their presence alone does not establish that the live backend, mail worker, monitoring, backups or privacy operations are deployed and verified. Read [release readiness](docs/RELEASE.md) and [deployment](docs/DEPLOYMENT.md) before operating a public service.

The codebase does not currently provide active chat, staff administration UI, public member directory, organization directory, working-group service, payment flow, MFA or automated moderation. Future participation and governance concepts on public pages are marked as in development or coming soon.

## Contributing and participation

The repository publishes its source at [github.com/SerhatSoruklu/codaris](https://github.com/SerhatSoruklu/codaris). For code changes, first read [AGENTS.md](AGENTS.md), the relevant architecture or feature documentation, and [SECURITY.md](SECURITY.md). Keep changes small, preserve the C17/WebAssembly and native C boundaries, use parameterized SQL, and never commit secrets. The project has no root `CONTRIBUTING.md`; use the contact page for general technical or collaboration enquiries. Do not assume that a GitHub contribution creates coalition membership or an organizational participation relationship.

Membership interest is expressed through the Join page when applications are enabled. The public topic library does not require membership. Organizational participation is not yet an active program; see [/ecosystem/organizational-participation/](https://codaris.org/ecosystem/organizational-participation/) for its current status.

## Further documentation

- [Account service and security design](docs/ACCOUNT-SERVICE.md)
- [Configuration](config/README.md)
- [Release runbook and operational limits](docs/RELEASE.md)
- [Deployment guide](docs/DEPLOYMENT.md)
- [Database and migrations](docs/DATABASES.md)
- [Developer topic library](docs/TOPIC-LIBRARY.md)
- [Security policy](SECURITY.md)
- [Privacy notice](https://codaris.org/privacy/) and [membership terms](https://codaris.org/terms/)
