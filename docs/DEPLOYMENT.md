# Isolated SSH deployment

Target: the owner's existing Linux/Nginx server. CODARIS uses a dedicated account, directory, virtual host, certificate and logs. Do not reuse another application’s process or database. Backend/database deployment is now required; private CODARIS configuration may use the expressly authorized Google Workspace SMTP identity. The deployment host address stays in local SSH configuration and is not committed.

## Current scope

The public frontend is static C/WebAssembly plus HTML/CSS, backed by the native account API and PostgreSQL. Deploy only the allowlisted frontend output produced by `scripts/package-site.py`; separately install the API, schema, backend environment and mail timer as described in [RELEASE.md](RELEASE.md). The instructions below cover static artifact activation and do not deploy the backend.

The existing server was inspected read-only. The steps below are prepared instructions, not a record of changes already applied. An administrator must provision the isolated account and virtual host once. Routine releases do not require sudo, Nginx reloads, or access to other applications.

## One-time administrator setup

1. Confirm that `codaris.org` and `www.codaris.org` point directly to the server's public address. Configure AAAA only if IPv6 works. Preserve unrelated DNS records. Confirm ports 80/443 allow direct clients; do not copy another site's Cloudflare-only restrictions or change its firewall rules. A shared server has a shared kernel and Nginx process: account isolation does not provide VM/container-level isolation.
2. Review the commands below and ensure the new account/group do not already exist. Create only CODARIS resources:

   ```bash
   sudo groupadd --system codaris-web
   sudo useradd --create-home --home-dir /srv/codaris/home --shell /bin/bash --gid codaris-web codaris-deploy
   sudo install -d -o codaris-deploy -g codaris-web -m 0750 /srv/codaris /srv/codaris/releases
   sudo usermod -a -G codaris-web www-data
   sudo install -d -o root -g root -m 0755 /var/lib/codaris/acme
   ```

   Do not give `codaris-deploy` sudo or membership of other application groups. `www-data` receives read access to CODARIS through its new supplementary group. Nginx must launch replacement workers after this change.
3. Generate a **dedicated** Ed25519 key on the administrator's workstation using `ssh-keygen -t ed25519`; use a passphrase and an SSH agent. Install only its public key in `/srv/codaris/home/.ssh/authorized_keys`, prefixed with `restrict`. The directory must be owned by `codaris-deploy`, mode 0700; the file mode 0600. Do not reuse another application's key, paste private keys into chat, or put a privileged server key in GitHub.
4. Add a local SSH alias, for example `codaris-rbx`, specifying the real host, `User codaris-deploy`, `IdentityFile` for that key, `IdentitiesOnly yes`, `ForwardAgent no`, and `StrictHostKeyChecking yes`. Verify the host-key fingerprint through the existing trusted server connection before adding it to known_hosts. Do not trust an unverified `ssh-keyscan` result.
5. Install `deploy/nginx/codaris-acme.conf` as `/etc/nginx/sites-available/codaris.conf` and enable only that site. Run `sudo nginx -t` and reload only if successful. Obtain the site's own certificate using the server's existing Certbot installation:

   ```bash
   sudo certbot certonly --webroot -w /var/lib/codaris/acme -d codaris.org -d www.codaris.org
   ```

   This requires working DNS for both names. Do not replace another site's certificate or ACME configuration. Verify the existing renewal timer and a successful renewal dry run for this certificate.
6. Build locally (`CODARIS_PRODUCTION=1 ./scripts/build-client.sh`). Install `build/deploy/security-headers.conf` as root-owned `/etc/nginx/snippets/codaris-security-headers.conf`, mode 0644. Replace the temporary virtual host with `deploy/nginx/codaris.conf`. Upload/activate the first release before enabling the final site; `scripts/deploy.sh` can stage it, but its last public health check will fail until TLS routing is enabled. Record that expected bootstrap failure and complete the public checks after enabling the site. Run `sudo nginx -t` before reloading. Confirm the new Nginx workers have the CODARIS read group.
7. Verify all other hosted applications before and after the reload. Keep backups of the specific CODARIS config files when updating them. Do not run blanket PM2 restarts or edit another site's configuration.

Nginx configuration is administrator-owned, outside deployment-writable directories. A new security-header policy requires a separately reviewed administrator install and reload. Normal content releases require neither.

## Routine release

Merge a reviewed PR only after the `Build and security checks` job succeeds. On Linux/WSL with the build tools installed:

```bash
git switch main
git pull --ff-only
./scripts/deploy.sh codaris-rbx
```

The script requires a clean worktree matching `origin/main`, builds with production indexing, validates pages, packages only public files, uploads over host-verified SSH, checks SHA-256 checksums, checks the published commit at `/version.txt`, saves the previous target and atomically switches `current`. It never copies source, environment files, database data or private keys. Failed uploads do not change `current`; failed public health checks are reported for investigation. Old releases are retained for explicit cleanup and rollback.

Windows users can build/check with `scripts/build-client.ps1` and `python tests/check-site.py`; use the same deployment command from WSL. GitHub Actions also creates a seven-day public-site artifact. It has read-only repository permissions, pinned actions and no deployment credentials. Do not add a public-repository runner to another application's server account.

## Rollback

Use the dedicated SSH account. Verify the printed previous target belongs under `/srv/codaris/releases/`, then run:

```bash
ssh codaris-rbx 'readlink /srv/codaris/current; readlink /srv/codaris/previous'
ssh codaris-rbx 'set -eu; target=$(readlink /srv/codaris/previous); case "$target" in /srv/codaris/releases/*/site) ;; *) exit 1;; esac; test -d "$target"; ln -s "$target" /srv/codaris/.rollback; mv -Tf /srv/codaris/.rollback /srv/codaris/current'
```

Investigate before deleting any releases. Rollback restores website content, not root-owned Nginx policy changes.

## Public verification

Check HTTPS certificate/renewal, canonical redirects, all routes, country search, globe interaction and missing-page 404s. Confirm `Content-Security-Policy`, `X-Content-Type-Options`, `X-Frame-Options`, HSTS, and correct `application/wasm` MIME type. Confirm `.git`, `.env`, source/config files and unsupported methods are inaccessible. Check the browser console for unexpected CSP violations. The policy permits Wasm compilation without JavaScript `unsafe-eval`; inline executable code, HTML script sinks, third-party scripts and form submission are blocked. This reduces attack surface; it does not guarantee immunity to XSS or network attacks.

## Future API and PM2

Design a long-running native C HTTP API first. Give it its own unprivileged runtime account, PM2 home/service, localhost port, logs, PostgreSQL role and credential file outside releases. Use PM2's `interpreter: none` for the compiled executable. Never share Coupyn/ChatPDM PM2 state or run the current health-check executable in a restart loop. Choose API authentication, TLS termination, limits and monitoring before exposing endpoints or enabling the join form.

References: [Nginx header inheritance](https://nginx.org/en/docs/http/ngx_http_headers_module.html), [CSP script policy](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Content-Security-Policy/script-src), [Emscripten settings](https://emscripten.org/docs/tools_reference/settings_reference.html#dynamic-execution), [GitHub Actions security](https://docs.github.com/en/actions/reference/security/secure-use).
