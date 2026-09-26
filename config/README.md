# Private environment configuration

The real local files are:

- `frontend.development.env`
- `frontend.production.env`
- `backend.development.env`
- `backend.production.env`

All four are ignored by Git. Never force-add them. A fresh checkout creates them with `python scripts/init-env.py`; existing files are preserved. This generator commits no credentials. Production credentials and database setup are machine-specific.

Use the same commands on Linux/WSL and Windows (use `python3` on Linux):

```text
python scripts/project.py dev check-env
python scripts/project.py dev build
python scripts/project.py dev migrate
python scripts/project.py dev run-server
python scripts/project.py dev run-client
python scripts/project.py dev mail
python scripts/project.py prod check-env
python scripts/project.py prod build
python scripts/project.py prod run-server
```

Run the API and frontend in separate terminals. `dev` and `prod` also accept `development` and `production`. Bash/PowerShell build and run scripts accept the mode as their first argument and default to development. `CODARIS_ENV` can select the default mode. The old `CODARIS_PRODUCTION=1` frontend build invocation remains supported when `CODARIS_ENV` is unset.

The selected file wins over inherited configuration. Frontend builds reject backend keys and remove inherited database/SMTP values. Native development and production binaries use separate directories (`build/linux/development`, `build/linux/production`, or `build/native/<mode>` on Windows). Production uses Release; development uses Debug.

Frontend `CODARIS_SITE_URL` is the canonical URL for SEO, not an API destination. All account requests use same-origin `/api/`. The development proxy forwards only to loopback. Production frontend hosting uses Nginx; `prod run-client` intentionally refuses to use the development server for production.

Development defaults to frontend `127.0.0.1:8081`, API `127.0.0.1:8080` and SMTP `127.0.0.1:1025`. To receive real local account/contact mail, configure a permitted SMTP account in `backend.development.env`; the full `./scripts/dev.sh` launcher drains its queue once per minute. Use `python scripts/project.py dev mail` for a one-time drain. Keep frontend ports and backend `CODARIS_ORIGIN` in sync; `check-env` checks the pair. Backend SMTP/authentication and database credentials are never frontend settings.

`CODARIS_MAIL_KEY` is an independent 32-byte random hex key per environment. Keep it private and retain it while pending encrypted mail exists. A machine-specific `CMAKE_PREFIX_PATH` or `CMAKE_TOOLCHAIN_FILE` may be configured for native dependencies. On Windows a fresh config uses local PostgreSQL TCP; on Linux it uses the private project socket.

Membership PDF exports embed the locally bundled DejaVu Sans font. Development builds resolve it from this checkout. Production installs must include `web/assets/fonts/DejaVuSans.ttf` as `/opt/codaris/DejaVuSans.ttf` (and its adjacent license file) with the API binary, or set `CODARIS_CREDENTIAL_FONT` to the installed font path. The font is not served to site visitors.

Deploy `backend.production.env` outside the public site, e.g. `/etc/codaris/backend.env`, with access restricted to the service account. See [release runbook](../docs/RELEASE.md). Never deploy the `config/` directory with frontend files.
