# Isolated SSH deployment

Target: the owner's existing Linux/Nginx server. CODARIS uses a dedicated account, directory, virtual host, certificate and logs. Do not reuse another application’s process or database. Backend/database deployment is now required; private CODARIS configuration may use the expressly authorized Google Workspace SMTP identity. The deployment host address stays in local SSH configuration and is not committed.

## Current scope

The public frontend is static C/WebAssembly plus HTML/CSS, backed by the native account API and PostgreSQL. Deploy only the allowlisted frontend output produced by `scripts/package-site.py`; separately install the API, schema, backend environment and mail timer as described in [RELEASE.md](RELEASE.md). The instructions below cover static artifact activation and do not deploy the backend.

The live server could not be reached from this workspace for read-only inspection: the configured `codaris-rbx` alias is absent and the hostname did not resolve. Public checks found `/api/health` returning 404. The steps below are prepared instructions, not a record of server changes already applied. An administrator must provision the isolated account and virtual host once. Routine releases use only the root-owned CODARIS helper; they do not grant a general sudo shell or touch another application's files.

## One-time administrator setup

1. Confirm the authoritative DNS, proxy/CDN mode and origin address for `codaris.org` and `www.codaris.org`; public requests currently arrive through Cloudflare. Configure AAAA only if IPv6 works. Check that Cloudflare DDoS/WAF policy matches the intended protection and that requests cannot bypass the edge through a publicly reachable origin. Because this host carries other applications, do not change shared firewall rules or restrict its public ports to Cloudflare addresses without checking the effect on every site. A shared server has a shared kernel and Nginx process: account isolation does not provide VM/container-level isolation.
2. Review the commands below and ensure the new account/group do not already exist. Create only CODARIS resources. The release and staging roots stay root-owned; the deployment account streams a tar archive to one restricted root helper and cannot write the live tree directly:

   ```bash
   sudo groupadd --system codaris-web
   sudo useradd --create-home --home-dir /home/codaris-deploy --shell /bin/bash --gid codaris-web codaris-deploy
   sudo install -d -o root -g codaris-web -m 0750 /srv/codaris /srv/codaris/releases /srv/codaris/.deploy-staging
   sudo usermod -a -G codaris-web www-data
   sudo install -d -o root -g root -m 0755 /var/lib/codaris/acme
   ```

   Keep `/srv/codaris`, `releases`, and `.deploy-staging` root-owned and not writable by the deployment account. `www-data` receives read access through `codaris-web`. Nginx must launch replacement workers after the group membership change. Do not give `codaris-deploy` membership in other application groups.
3. Create a dedicated GitHub Actions Ed25519 key on a trusted workstation with `ssh-keygen -t ed25519 -N '' -C codaris-github-deploy`. This non-interactive key is limited to a deployment-only account. Add its public key to `/home/codaris-deploy/.ssh/authorized_keys` with the `restrict` option; set `.ssh` to owner `codaris-deploy`, mode 0700, and `authorized_keys` to mode 0600. Keep its private half only in the GitHub `production` environment secret. Do not reuse another application's key, paste private keys into chat, or install a privileged server key in GitHub.
4. Install the single root helper and its sudoers rule from a reviewed checkout:

   ```bash
   sudo install -o root -g root -m 0755 deploy/codaris-deploy-release /usr/local/sbin/codaris-deploy-release
   sudo install -o root -g root -m 0440 deploy/codaris-deploy.sudoers /etc/sudoers.d/codaris-deploy
   sudo visudo -cf /etc/sudoers.d/codaris-deploy
   ```

   The helper accepts only `activate RELEASE` and `rollback RELEASE`, validates the release archive and checksums, installs only the three CODARIS Nginx files, runs the full `nginx -t`, and reloads Nginx only when those files change and validation succeeds. It activates only a CODARIS site release. It does not provide a general root shell or sudo access to other commands. The archive is streamed over SSH into root-owned staging; the deployment account cannot alter it during activation.
5. Verify the server host-key fingerprint from the existing trusted server session with `sudo ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub`. Configure the GitHub `production` environment with the host and port, this verified host-key line, and the private deployment key. Do not trust an unverified `ssh-keyscan` result. For local admin SSH, add a separate `codaris-rbx` alias using `User codaris-deploy`, `IdentitiesOnly yes`, `ForwardAgent no`, and `StrictHostKeyChecking yes`.
6. Install `deploy/nginx/codaris-acme.conf` as `/etc/nginx/sites-available/codaris.conf` and enable only that site. Run `sudo nginx -t` and reload only if successful. Obtain the site's own certificate using the server's existing Certbot installation:

   ```bash
   sudo certbot certonly --webroot -w /var/lib/codaris/acme -d codaris.org -d www.codaris.org
   ```

   This requires working DNS for both names. Do not replace another site's certificate or ACME configuration. Verify the existing renewal timer and a successful renewal dry run for this certificate.
7. Build locally (`CODARIS_PRODUCTION=1 ./scripts/build-client.sh`). Install `build/deploy/security-headers.conf` as root-owned `/etc/nginx/snippets/codaris-security-headers.conf` and `build/deploy/member-routes.conf` as root-owned `/etc/nginx/snippets/codaris-member-routes.conf`, both mode 0644. The member-route snippet is generated from `access: "member"` in `web/pages.json`. Create `/etc/nginx/snippets/codaris-cloudflare-real-ip.conf` from the current official Cloudflare IPv4 and IPv6 CIDRs, containing one `set_real_ip_from <CIDR>;` per range plus `real_ip_header CF-Connecting-IP;` and `real_ip_recursive on;`. Keep it root-owned mode 0644 and update it when Cloudflare changes its published ranges. Never trust `CF-Connecting-IP` from arbitrary peers. The API per-IP limits and the overwritten `X-Real-IP` are correct only after this trusted-proxy policy is active. Replace the temporary virtual host with `deploy/nginx/codaris.conf`. Deploy the API, schema and mail worker first; `/api/health` must report `page_access_api: 1`. The automated deployment helper updates the CODARIS vhost and generated snippets after the initial certificates, Cloudflare include, enabled site and API are in place. Run `sudo nginx -t` before any initial manual reload. Confirm the new Nginx workers have the CODARIS read group.
8. Before enabling the workflow, seed `/srv/codaris/releases/bootstrap/site` with a verified copy of the currently served CODARIS site, owned by `root:codaris-web`, directories mode 0750 and files mode 0640, then create `/srv/codaris/current` as a symlink to that `site` directory. First inspect the existing document root and copy only the public site files; do not guess the source path or copy secrets. This rollback point is required: the helper rejects the first automated deployment if there is no current release to restore.
9. Verify all other hosted applications before and after the initial reload. Keep backups of the specific CODARIS config files when updating them. Do not run blanket PM2 restarts or edit another site's configuration.

Nginx configuration is root-owned, outside deployment-writable directories. The deployment helper installs only CODARIS config and generated snippets; all changes are checked with `nginx -t` before a shared Nginx reload. Review the generated member-route snippet when route access metadata changes.

## GitHub Actions production deployment

The `Verify` workflow now builds and checks the site and native backend on pull requests targeting `main`. It also validates the CODARIS vhost with `nginx -t` in an isolated configuration, and packages only allowlisted site files plus `deploy/nginx/codaris.conf` and the generated CODARIS snippets. On a successful push/merge to `main`, a separate production-environment job streams that package to the dedicated deployment account. It never receives a general server password or root key. The root helper activates the CODARIS files and reloads shared Nginx only when its configuration changed and `nginx -t` passed.

Before enabling the workflow, configure the GitHub repository:

1. `main` is configured to require a pull request and the `Build and security checks` result, to keep the branch current before merge, to resolve review conversations, and to block force pushes and deletion. Approval count is zero so a single maintainer can merge after CI passes.
2. The GitHub Actions environment `production` exists and allows deployment jobs from `main` only. A required reviewer is optional; without one, merge to `main` deploys automatically.
3. Add environment variable `CODARIS_DEPLOY_HOST` (the server's verified SSH host/IP), optional `CODARIS_DEPLOY_PORT` (defaults to `22`), and `CODARIS_DEPLOY_KNOWN_HOSTS` (the host-key entry verified against the server fingerprint).
4. Add environment secret `CODARIS_DEPLOY_PRIVATE_KEY` (the private half of the dedicated `codaris-deploy` key). Do not generate or commit a substitute key in the repository.
5. Set `CODARIS_DEPLOY_ENABLED` to `true` only after the server helper, bootstrap release, API health route and deployment key have all been verified. Until then, merges run build/security checks and skip the deployment job.

No production environment secret or deployment variables are configured yet. The workspace also has no `codaris-rbx` SSH alias, and `/api/health` currently returns HTTP 404. The deployment job is therefore gated off by `CODARIS_DEPLOY_ENABLED`; merge CI can pass without making a live change. Once setup is verified and the gate is enabled, a deployment must report a matching public `/version.txt`, the expected security headers, and a healthy API (`contact_api: 1`, `credential_api: 2`, `page_access_api: 1`). If public verification fails, the workflow asks the same guarded helper to restore the preceding site and CODARIS Nginx config.

For an authorized manual release from Linux/WSL, after the same setup:

```bash
git switch main
git pull --ff-only
./scripts/deploy.sh codaris-rbx
```

The script requires a clean worktree matching `origin/main`, builds and checks the production site, validates Nginx syntax, packages only allowlisted site and CODARIS configuration files, and streams the archive over host-verified SSH. The root helper verifies all checksums before installing anything. No source, environment files, database data or private keys are uploaded. Failed archive validation or Nginx checks leave the current site/config active. Old releases and root-owned Nginx backups are retained for explicit cleanup and rollback.

Windows users can build/check with `scripts/build-client.ps1` and `python tests/check-site.py`; use the deployment command from WSL. GitHub Actions uses read-only repository permissions, pinned actions, a production environment and a dedicated non-privileged key. Do not add a public-repository runner to this shared server or another application's account.

## Rollback

Use the dedicated SSH account and the release id printed by the workflow:

```bash
ssh codaris-rbx 'readlink /srv/codaris/current; readlink /srv/codaris/previous'
ssh codaris-rbx 'sudo -n /usr/local/sbin/codaris-deploy-release rollback RELEASE_ID'
```

Rollback restores the preceding site plus the CODARIS Nginx files saved for that release, checks the full Nginx configuration, and reloads shared Nginx only if validation succeeds. Investigate before deleting releases or backups.

## Public verification

Check HTTPS certificate/renewal, canonical redirects, all routes, country search, globe interaction and missing-page 404s. Confirm `Content-Security-Policy`, `X-Content-Type-Options`, `X-Frame-Options`, HSTS, and correct `application/wasm` MIME type. Confirm `.git`, `.env`, source/config files and unsupported methods are inaccessible. Check the browser console for unexpected CSP violations. The policy permits Wasm compilation without JavaScript `unsafe-eval`; inline executable code, HTML script sinks, third-party scripts and form submission are blocked. This reduces attack surface; it does not guarantee immunity to XSS or network attacks.

## API process lifecycle

The repository already contains the long-running native C API and the one-shot mail worker. Run them as the dedicated `codaris` operating-system account using `deploy/codaris-api.service` and `deploy/codaris-mail.service` plus `deploy/codaris-mail.timer`. The API binds only to `127.0.0.1`; Nginx terminates HTTPS and proxies `/api/` to it. The service units use systemd restart behavior and sandboxing. `scripts/dev.sh` is only a local-development launcher; systemd starts production services at boot and keeps them running. PM2 is not needed and must not be shared with Coupyn, ChatPDM or other applications.

The API's binary, font, libraries and backend environment are installed separately from static releases. Backend upgrades require building the matching production binary, staging it under `/opt/codaris`, applying any reviewed migration with a backup in place, and restarting only `codaris-api.service`. The mail timer runs `codaris-mail.service` once per minute. Check `systemctl status codaris-api.service codaris-mail.timer`, `journalctl -u codaris-api.service`, and the public `/api/health` response after each backend release. Never enable account registration until API health reports `page_access_api: 1` and the browser-facing account flows have passed.

## Compression and availability limits

Nginx gzip is enabled for HTML and public static CSS, JavaScript, WebAssembly, SVG and XML. Personalized JSON API responses are deliberately not in `gzip_types`; compressing responses that combine secrets with attacker-controlled data can create side-channel risks. Gzip is built into normal Nginx packages, so CODARIS does not require a third-party Brotli module. Consider Brotli only if the installed Nginx package already provides a maintained module and measurements show a material benefit. Test both transfer size and CPU under representative traffic before changing compression settings.

The API location applies a per-client request rate and a concurrent-connection cap. These reduce simple floods and protect the single-threaded API, but IP-based limits affect people sharing a NAT and do not stop distributed denial-of-service traffic. Keep the host provider's network-level DDoS protection and firewall in place; do not change shared-host firewall rules for CODARIS without assessing every hosted application. A public availability monitor should check HTTPS, `/api/health`, certificate expiry and mail-worker failures without submitting real registrations.

The included limits are starting safeguards, not capacity guarantees. Load-test the expected traffic and tune them based on observed legitimate bursts, PostgreSQL capacity and Argon2id cost. Keep test traffic away from the live account database.

References: [Nginx header inheritance](https://nginx.org/en/docs/http/ngx_http_headers_module.html), [Nginx trusted real-IP module](https://nginx.org/en/docs/http/ngx_http_realip_module.html), [Cloudflare visitor-IP restoration and trusted ranges](https://developers.cloudflare.com/support/troubleshooting/restoring-visitor-ips/restoring-original-visitor-ips/), [CSP script policy](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Content-Security-Policy/script-src), [Emscripten settings](https://emscripten.org/docs/tools_reference/settings_reference.html#dynamic-execution), [GitHub Actions security](https://docs.github.com/en/actions/reference/security/secure-use).
