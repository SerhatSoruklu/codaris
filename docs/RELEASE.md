# Account release runbook

## Implemented locally

C17/libmicrohttpd API; parameterized PostgreSQL; Argon2id passwords; login/logout; email verification and immediate email replacement with reverification; password change/recovery; editable name, country, role, external URLs and avatars; immutable original joining reason; persistent topic progress; aggregate verified-member counts. Automatic membership after email verification. Staff approval and chat are unavailable, not simulated.

## Environment separation

The actual files are `config/frontend.development.env`, `config/frontend.production.env`, `config/backend.development.env` and `config/backend.production.env`. They are ignored by Git. `python3 scripts/init-env.py` creates missing files without overwriting existing configuration. See [config/README.md](../config/README.md) for the supported public and private settings.

Use `python3 scripts/project.py dev ACTION` or `python3 scripts/project.py prod ACTION`. The selector loads exactly the chosen component/environment, strips inherited database/SMTP settings and rejects private settings in frontend files. Native Debug/Release builds have separate mode directories. Frontend HTTP requests always use same-origin `/api/`; the canonical URL is not a backend destination.

```bash
bash scripts/db-local.sh start
python3 scripts/project.py dev check-env
python3 scripts/project.py dev migrate
python3 scripts/project.py dev build
python3 scripts/project.py dev run-server
# Separate terminals:
python3 scripts/project.py dev run-client
python3 scripts/project.py dev mail
# Release build:
python3 scripts/project.py prod build
```

Development defaults: frontend `http://127.0.0.1:8081`, API `127.0.0.1:8080`, local SMTP capture port 1025. Keep the frontend ports and backend origin matched. Production uses Nginx TLS, its own database/password file and Google Workspace SMTP; the development proxy cannot be selected for production hosting.

On Windows use `python` in the same commands. Install native dependencies through vcpkg and configure its CMake toolchain. Native binaries live in `build/native/<mode>/<Debug-or-Release>/`; Linux binaries live in `build/linux/<mode>/`. Native Windows execution still requires verification on Windows.

## Google Workspace SMTP

Matched to the existing Serhat Soruklu application:

- `CODARIS_SMTP_URL=smtp://smtp.gmail.com:587`
- `CODARIS_SMTP_USER=admin@coupyn.com`
- `CODARIS_MAIL_FROM=admin@coupyn.com`, display name CODARIS.
- `CODARIS_SMTP_PASSWORD` is the Workspace-permitted app password, stored only in the backend environment.
- Production libcurl requires STARTTLS and verifies the server certificate. Ordinary account passwords are not a substitute for a permitted app password.
- Reply-To is `contact@codaris.org`.

Inbound forwarding from `no-reply@codaris.org` does not grant permission to send from it. To switch From to that address, first configure and verify it as an authorized Google send-as alias, and verify SPF/DKIM/DMARC alignment. Do not change other projects' SMTP configuration.

References: [Google application SMTP setup](https://support.google.com/a/answer/176600), [Google send-as aliases](https://support.google.com/mail/answer/22370).

`--mail-once` drains up to 20 events. Install `deploy/codaris-mail.service` and `.timer` to run it every minute; monitor failures and exhausted retries. Google Workspace sending limits are shared with other applications using the mailbox; check remaining capacity before a public beta launch. It also cleans expired sessions/tokens, one-day rate-limit records and 30-day email records. Retry delivery is at-least-once with stable Message-ID and single-use actions. This cannot guarantee exactly-once SMTP delivery.

## Production rollout

1. Provision an isolated Linux account, PostgreSQL database and runtime role. Separate migration ownership from runtime privileges; grant only required table/sequence access. Set restrictive environment/password-file permissions. Never use the local development cluster role in production.
2. Apply sequential migrations with `scripts/db-migrate.py` using production PG* variables. Migration 001 remains unchanged. Take a backup and verify restore before rollout. Run only one migration process at a time.
3. Install the native binary and shared libraries at `/opt/codaris/`, backend environment at `/etc/codaris/backend.env`, and systemd units in `deploy/`. Configure PostgreSQL statement/parameter logging so credentials and verification material are not logged.
4. Install the updated `deploy/nginx/codaris.conf` after certificates exist. `/api/` proxies to loopback, replaces X-Real-IP and disables API access logs. Static pages allow GET/HEAD; API request bodies are capped at 96 KiB. The supplied reverse-proxy limit is 5 requests/second with a burst of 20 per IP; native account limits also persist in PostgreSQL. The routine `scripts/deploy.sh` publishes only frontend assets and does not install the API, database or timers. A release is not account-ready until the service is running, migrations are applied, and public `/api/health` returns HTTP 200 with JSON.
5. Build production client (`python3 scripts/project.py prod build-client`), run checks, package with `scripts/package-site.py`. Publish only the allowlisted site artifact. Configure log rotation and backup retention and confirm the privacy notice accurately describes the deployed providers and transfers.
6. Validate `/api/health`, real signup, verification, login, avatar/settings persistence, email change, reset and logout through public HTTPS. Verify delivered email headers and sender identity, not just SMTP authentication. Check queue retry alerts, backup restoration and rollback. Do not announce release until this succeeds.

Registration becomes real when API deployment is reachable: do not publish the account UI with an absent or unmigrated backend. Service failures produce honest errors, never fake successful submissions. A non-JSON response from `/api/` indicates the proxy/site is misconfigured or the service is unavailable; check Nginx routing and `codaris-api.service` before retrying registration.

## Checks

```bash
python3 scripts/project.py dev build-server
CODARIS_TEST_BINARY=build/linux/development/codaris_server python3 tests/check-accounts.py
bash scripts/build-client.sh
python3 tests/check-membership-build.py
python3 tests/check-site.py
python3 tests/check-topic-library.py
```

The account integration test creates and drops its own random PostgreSQL database and captures SMTP locally. It never sends mail to real users. Test role requires CREATEDB locally. Production credentials must not be used for it.

## Operational limits

The HTTP server currently uses one polling thread and bounded connections; password hashing is intentionally costly. Load-test against expected launch traffic and configure proxy connection/request limits. There is no MFA, staff authorization UI, shared chat, or automated abuse moderation. These are not represented as functioning services. Terms/privacy contain the operator-supplied details and need to remain consistent with actual operational practices. Account deletion/access requests currently use the published privacy mailbox.

## Local release candidate validation — 24 September 2026

Version: 1.0.0-beta.1. These results describe this workspace, not a live deployment.

- Native C17 build passed; the account integration suite also passed under AddressSanitizer and UndefinedBehaviorSanitizer, with warnings treated as errors.
- PostgreSQL/SMTP capture tests passed for transaction rollback on queue failure, password hashing, signup/login/logout, verification/replay rejection, immediate email replacement, old-session revocation, recovery, URL validation, immutable motivation, image size validation, avatar persistence, learning progress, aggregates and request limits. Production mode tests checked Secure cookies and origin rejection.
- Both frontend build modes and all 39 generated documents passed site checks. Language, framework, database and topic-library integrity tests passed.
- Browser checks passed for signup/login, profile and avatar persistence, read-only application response, persisted topic progress and production document behavior with no JavaScript page errors. Homepage Beta V1 and 23M target checked at desktop and 375-pixel mobile widths without horizontal overflow.
- Nginx config test passed with test certificates, unprivileged ports and local paths substituted. This does not validate the production host’s certificates, ports or installed configuration.
- Google Workspace STARTTLS and authentication succeeded using the selected mailbox; no live message was sent. Local SMTP tests verified the multipart email contents and retries.
- Public artifact inspection found no backend SMTP password or mail-encryption key. Private environment files are ignored by Git.

Not completed: installing the service/database on the production host, verifying a real delivered verification email and sender headers through the live site, production backup/restore and capacity checks, and native Windows execution. The public site has not been deployed by this work. Use the rollout steps above before announcing release.

Real-environment follow-up: `tests/check-environments.py` passed; both `project.py dev/prod build-client` selected their corresponding frontend flags; Debug and Release native builds and account integration suites passed separately. Real `.env` files remain ignored and existing Google credentials were preserved only in the private production backend configuration.
