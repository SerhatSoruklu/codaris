# Isolated SSH deployment

Target: the owner's existing Linux/Nginx server. CODARIS uses a dedicated account, directory, virtual host, certificate and logs. Do not reuse another application’s process or database. Backend/database deployment is now required; private CODARIS configuration may use the expressly authorized Google Workspace SMTP identity. The deployment host address stays in local SSH configuration and is not committed.

## Current scope

The public frontend is static C/WebAssembly plus HTML/CSS, backed by the native account API and PostgreSQL. The allowlisted release package contains the site, native API binary, credential font, and CODARIS Nginx files. Merges to `main` deploy the API and site together, then verify the API and public site. Database migrations remain a separate reviewed, backed-up operation; deployments never run them automatically.

Read-only SSH inspection was performed on 29 September 2026 using the supplied `coupyn-rbx` Tailscale address. The host is shared with Coupyn and ChatPDM. The administrator has since created the CODARIS service/deploy accounts, groups, release roots and ACME/backup directories; installed `libmicrohttpd.so.12`; installed the root-owned deployment helper, sudoers rule, API/mail systemd units and dedicated deploy public key; then reloaded systemd. PostgreSQL 16 is running locally. The `codaris` database and non-privileged `codaris_migrator`/`codaris_app` login roles already existed; the database and `app` schema belong to `codaris_migrator`, and read-only inspection on 29 September found no non-system tables or views. The migration state still needs confirmation before enabling account flows. On 30 September, the administrator added a temporary API HTTP-01 vhost, confirmed the challenge path publicly, and issued a separate `api.codaris.org` certificate expiring 29 December 2026. The Certbot timer is active and an API-only Nginx reload deploy hook is installed. A staging renewal dry run returned `rateLimited` / `Service busy`; this did not invalidate the issued certificate, and the deploy hook was not exercised by that failed dry run. The API service is active and its local health endpoint reports `Ready` with the expected API versions. The permanent CODARIS Nginx config is not yet active: the root vhost still proxies `/api/` on `codaris.org`, `api.codaris.org` has only the temporary HTTP vhost, and public API HTTPS returns 525. Deploy the site, API binary and permanent Nginx config together through the guarded release process, then verify public HTTPS health. The server's Tailscale address is private and must not be used as a public Cloudflare origin. Routine releases use only the root-owned CODARIS helper; they do not grant a general sudo shell or touch another application's files.

## One-time administrator setup

1. Keep Cloudflare proxying the public web records: current responses and DNS show `codaris.org` is already proxied through Cloudflare. In the Cloudflare dashboard, confirm whether the origin record is a public server IP or a Tunnel target; public observations cannot reveal that origin. Configure AAAA only if IPv6 works. Check that Cloudflare DDoS/WAF policy matches the intended protection and that requests cannot bypass the edge through a publicly reachable origin. Because this host carries other applications, do not change shared firewall rules or restrict its public ports to Cloudflare addresses without checking the effect on every site. A shared server has a shared kernel and Nginx process: account isolation does not provide VM/container-level isolation.
2. Review the commands below and ensure the new accounts/groups do not already exist. Create only CODARIS resources. The release and staging roots stay root-owned; the deployment account streams a tar archive to one restricted root helper and cannot write the live tree directly:

   ```bash
   sudo groupadd --system codaris-web
   sudo useradd --create-home --home-dir /home/codaris-deploy --shell /bin/bash --gid codaris-web codaris-deploy
   sudo groupadd --system codaris
   sudo useradd --system --gid codaris --home-dir /var/lib/codaris --shell /usr/sbin/nologin codaris
   sudo install -d -o root -g codaris-web -m 0750 /srv/codaris /srv/codaris/releases /srv/codaris/.deploy-staging
   sudo install -d -o root -g codaris -m 0750 /opt/codaris /opt/codaris/releases /opt/codaris/.deploy-staging
   sudo usermod -a -G codaris-web www-data
   sudo install -d -o root -g root -m 0755 /var/lib/codaris/acme
   sudo install -d -o root -g root -m 0700 /var/backups/codaris-nginx
   sudo apt-get install libmicrohttpd12t64
   ```

   Keep `/srv/codaris`, `releases`, and `.deploy-staging` root-owned and not writable by the deployment account. `www-data` receives read access through `codaris-web`. Nginx must launch replacement workers after the group membership change. Do not give `codaris-deploy` membership in other application groups.
3. The dedicated GitHub Actions Ed25519 key has been generated on the owner's workstation. Install only its public key in `/home/codaris-deploy/.ssh/authorized_keys` with the `restrict` option; set `.ssh` to owner `codaris-deploy`, mode 0700, and `authorized_keys` to mode 0600. Keep the private key only in the GitHub `production` environment secret. Do not reuse another application's key, paste private keys into chat, or install a privileged server key in GitHub.
4. Install the root helper, its sudoers rule, and the API/mail systemd units from a reviewed checkout:

   ```bash
   sudo install -o root -g root -m 0755 deploy/codaris-deploy-release /usr/local/sbin/codaris-deploy-release
   sudo install -o root -g root -m 0440 deploy/codaris-deploy.sudoers /etc/sudoers.d/codaris-deploy
   sudo install -o root -g root -m 0644 deploy/codaris-api.service /etc/systemd/system/codaris-api.service
   sudo install -o root -g root -m 0644 deploy/codaris-mail.service /etc/systemd/system/codaris-mail.service
   sudo install -o root -g root -m 0644 deploy/codaris-mail.timer /etc/systemd/system/codaris-mail.timer
   sudo visudo -cf /etc/sudoers.d/codaris-deploy
   sudo systemctl daemon-reload
   ```

   The helper accepts only `activate RELEASE` and `rollback RELEASE`, validates the release archive and checksums, installs the allowlisted site/API files and three CODARIS Nginx files, checks/restarts only `codaris-api.service`, and reloads shared Nginx only after `nginx -t` passes. Site and API pointers are switched together and restored together if local API health or Nginx validation fails. It does not provide a general root shell or sudo access to other commands. The archive is streamed over SSH into root-owned staging; the deployment account cannot alter it during activation.
5. Verify the server host-key fingerprint from the existing trusted server session with `sudo ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub`. Verify a `ssh-keyscan -t ed25519` result against that fingerprint before adding its line to GitHub's `CODARIS_DEPLOY_KNOWN_HOSTS` variable. Do not trust an unverified scan. The deployment workflow joins the private Tailscale network and uses the server's Tailscale IP or MagicDNS name for SSH; do not expose SSH publicly for GitHub Actions. For local admin SSH, add a separate `codaris-rbx` alias using `User codaris-deploy`, `IdentitiesOnly yes`, `ForwardAgent no`, and `StrictHostKeyChecking yes`.
6. For the initial CODARIS setup, install `deploy/nginx/codaris-acme.conf` as `/etc/nginx/sites-available/codaris.conf` and enable only that site. It serves HTTP challenges for the root, www and API names. Run `sudo nginx -t` and reload only if successful. Obtain the site's own certificate using the server's existing Certbot installation:

   ```bash
   sudo certbot certonly --webroot -w /var/lib/codaris/acme -d codaris.org -d www.codaris.org
   ```

   This requires working DNS for both names. Do not replace another site's certificate or ACME configuration. If the root-domain vhost is already live, do not replace it with the HTTP-only bootstrap file. Add an API-only temporary vhost without changing the live site:

   ```bash
   sudo tee /etc/nginx/conf.d/codaris-api-acme.conf >/dev/null <<'NGINX'
   server {
       listen 80;
       listen [::]:80;
       server_name api.codaris.org;
       server_tokens off;
       location ^~ /.well-known/acme-challenge/ {
           root /var/lib/codaris/acme;
           try_files $uri =404;
       }
       location / { return 503; }
   }
   NGINX
   sudo nginx -t && sudo systemctl reload nginx
   sudo install -d -o root -g root -m 0755 /var/lib/codaris/acme/.well-known/acme-challenge
   printf '%s\n' codaris-api-acme-probe | sudo tee /var/lib/codaris/acme/.well-known/acme-challenge/codaris-api-probe >/dev/null
   curl --fail http://api.codaris.org/.well-known/acme-challenge/codaris-api-probe
   sudo rm /var/lib/codaris/acme/.well-known/acme-challenge/codaris-api-probe
   ```

   Continue only if the public `curl` prints `codaris-api-acme-probe`. Then issue and verify the separately named API certificate:

   ```bash
   sudo certbot certonly --webroot -w /var/lib/codaris/acme --cert-name api.codaris.org -d api.codaris.org
   ```

   Add a deploy hook so Nginx reloads only after the API certificate renews:

   ```bash
   sudo install -d -o root -g root -m 0755 /etc/letsencrypt/renewal-hooks/deploy
   sudo tee /etc/letsencrypt/renewal-hooks/deploy/codaris-api-reload >/dev/null <<'HOOK'
   #!/bin/sh
   case " ${RENEWED_DOMAINS:-} " in
       *" api.codaris.org "*) exec /usr/bin/systemctl reload nginx ;;
   esac
   exit 0
   HOOK
   sudo chmod 0755 /etc/letsencrypt/renewal-hooks/deploy/codaris-api-reload
   sudo certbot renew --dry-run --run-deploy-hooks --cert-name api.codaris.org
   systemctl list-timers --all certbot.timer
   ```

   The inspected host already has Certbot's systemd timer active; no additional renewal timer is needed. Remove the temporary vhost as part of activating the permanent `codaris.conf`, which provides the HTTP challenge location and HTTPS API vhost. Validate and reload with the permanent config in place; never enable both API vhosts together.
7. The existing `codaris` database and its separate `codaris_migrator` and `codaris_app` login roles were inspected on 29 September 2026. The database and `app` schema belong to the non-superuser migrator; no non-system tables, views or sequences were found. Do not recreate these resources. Take and verify a database backup, then apply the reviewed migrations manually through the workstation's database tunnel using `scripts/db-live-migrate.sh`. That script keeps ownership with `codaris_migrator` and grants only the API/mail operations to runtime role `codaris_app`. Install `/etc/codaris/backend.env` and `/etc/codaris/pgpass` with permissions that allow only the service account/required systemd reader. Keep passwords, SMTP credentials and `CODARIS_MAIL_KEY` out of GitHub deployment artifacts. Use the release artifact's API binary and font to create `/opt/codaris/releases/bootstrap`, set it root-owned/readable by group `codaris`, and point `/opt/codaris/current` at it. Start `codaris-api.service` and require local `http://127.0.0.1:8080/api/health` to return all three API versions before continuing. Enable `codaris-mail.timer` only after a real Workspace app password and sender configuration are verified.
8. Install `build/deploy/security-headers.conf` as root-owned `/etc/nginx/snippets/codaris-security-headers.conf` and `build/deploy/member-routes.conf` as root-owned `/etc/nginx/snippets/codaris-member-routes.conf`, both mode 0644. The member-route snippet is generated from `access: "member"` in `web/pages.json`. Create `/etc/nginx/snippets/codaris-cloudflare-real-ip.conf` from the current official Cloudflare IPv4 and IPv6 CIDRs, containing one `set_real_ip_from <CIDR>;` per range plus `real_ip_header CF-Connecting-IP;` and `real_ip_recursive on;`. Keep it root-owned mode 0644 and update it when Cloudflare changes its published ranges. Never trust `CF-Connecting-IP` from arbitrary peers. The API per-IP limits and overwritten `X-Real-IP` are correct only after this trusted-proxy policy is active.
9. Replace the temporary virtual host with `deploy/nginx/codaris.conf` only after the certificate, trusted-proxy include, and generated snippets are installed. Run `sudo nginx -t` before any initial manual reload. Confirm the new Nginx workers have the CODARIS read group.
10. The workflow retains a `codaris-production-release` artifact for seven days after each successful `main` build. From the trusted workstation, download that artifact with GitHub CLI (`gh run download RUN_ID --name codaris-production-release --dir /tmp/codaris-bootstrap`) or GitHub's Actions UI, then verify `sha256sum --check SHA256SUMS`. Transfer the allowlisted `site/` and `api/` directories to the administrator for bootstrap; the artifact contains no source or secrets, so no repository clone on the server is needed.
11. Install the first public site under `/srv/codaris/releases/bootstrap/site`, owned by `root:codaris-web`, directories mode 0750 and files mode 0640, then create `/srv/codaris/current` as a symlink to it. Install the bootstrap API files under `/opt/codaris/releases/bootstrap`, owned by `root:codaris`, with the API binary mode 0750 and font/license mode 0640; create `/opt/codaris/current` as a symlink to it. These known-good site/API pointers are the rollback target for the first automatic deployment.
12. Verify all other hosted applications before and after the initial reload. Keep backups of the specific CODARIS config files when updating them. Do not run blanket PM2 restarts or edit another site's configuration.

Nginx configuration is root-owned, outside deployment-writable directories. The deployment helper installs only CODARIS config and generated snippets; all changes are checked with `nginx -t` before a shared Nginx reload. Review the generated member-route snippet when route access metadata changes.

## GitHub Actions production deployment

The `Verify` workflow builds and checks the site and native backend on pull requests targeting `main`. It validates the CODARIS vhost with `nginx -t` in an isolated configuration, then packages only allowlisted site files, the API binary/font, and the CODARIS Nginx files. Ubuntu package archives used by the build are cached weekly; `apt-get update` still refreshes package indexes on every run, and the current versions selected from those indexes are installed. A cache miss can take longer while the Emscripten toolchain packages are downloaded. On a successful push/merge to `main`, a separate production-environment job streams that package to the dedicated deployment account. It never receives a general server password or root key. The root helper verifies the API locally after restart, switches the site/API release pointers together, and reloads shared Nginx only when its configuration changed and `nginx -t` passed. The workflow then checks the public site, security headers, and API health. It never applies database migrations.

Before enabling the workflow, configure the GitHub repository:

1. `main` is configured to require a pull request and the `Build and security checks` result, to keep the branch current before merge, to resolve review conversations, and to block force pushes and deletion. Approval count is zero so a single maintainer can merge after CI passes.
2. The GitHub Actions environment `production` exists and allows deployment jobs from `main` only. A required reviewer is optional; without one, merge to `main` deploys automatically.
3. Add environment variable `CODARIS_DEPLOY_HOST` (the server's Tailscale IP or MagicDNS name), optional `CODARIS_DEPLOY_PORT` (defaults to `22`), and `CODARIS_DEPLOY_KNOWN_HOSTS` (the host-key entry verified against the server fingerprint).
4. Add environment secret `CODARIS_DEPLOY_PRIVATE_KEY` (the private half of the dedicated `codaris-deploy` key). Do not generate or commit a substitute key in the repository.
5. Add environment secrets `CODARIS_TAILSCALE_OAUTH_CLIENT_ID` and `CODARIS_TAILSCALE_OAUTH_SECRET`. Create a Tailscale OAuth client with `Devices > Core > Write` and `Keys > Auth Keys > Write`, restricted to the `tag:codaris-ci` tag; the workflow uses it to add a temporary tagged runner to the tailnet. Add only a network access rule allowing `tag:codaris-ci` to reach the CODARIS server's Tailscale IP on TCP port `CODARIS_DEPLOY_PORT` (normally 22). Keep Tailscale SSH disabled on the deployment server (`sudo tailscale set --ssh=false`) so the workflow authenticates to standard OpenSSH with its dedicated private key and pinned host key. Before disabling Tailscale SSH, verify that an administrator can still connect using a normal OpenSSH key. Do not add `tag:codaris-ci` to the Tailscale SSH policy: that would allow tagged CI nodes to log in as `codaris-deploy` without the dedicated SSH key. Review the whole policy first: an existing broad network access rule would still grant broader access. Do not replace the shared policy or change access for other devices without checking its effects.
6. Set `CODARIS_DEPLOY_ENABLED` to `true` as a **repository-level Actions variable** only after the server helper, bootstrap site/API releases, healthy API, database and schema, SSH key, Tailscale credentials and narrow tailnet rule have all been verified. The deploy job checks this value before its `production` environment is attached, so an environment-level variable is not available to that condition and causes the job to be skipped. Until the repository-level gate is true, merges run build/security checks and skip deployment. Before merging a change that requires a schema change, take a verified backup and apply the reviewed migration manually; make migrations backward-compatible with the currently running API so deployment rollback remains possible.

Keep `CODARIS_DEPLOY_ENABLED` unset or `false` until the production prerequisites above are verified. Once enabled at repository scope, a deployment must report a matching public `/version.txt`, the expected security headers, and a healthy `https://api.codaris.org/api/health` (`contact_api: 1`, `credential_api: 2`, `page_access_api: 1`). If public verification fails, the workflow asks the same guarded helper to restore the preceding site, API and CODARIS Nginx config. Database migrations remain untouched by that rollback.

For an authorized manual release from Linux/WSL, after the same setup:

```bash
git switch main
git pull --ff-only
./scripts/deploy.sh codaris-rbx
```

The script requires a clean worktree matching `origin/main`, builds and checks the production site/API, validates Nginx syntax, packages only allowlisted site, API and CODARIS configuration files, and streams the archive over host-verified SSH. The root helper verifies all checksums before installing anything. No source, environment files, database data or private keys are uploaded. Failed archive validation, API health or Nginx checks restore the previous site/API/config. Old releases and root-owned Nginx backups are retained for explicit cleanup and rollback.

Windows users can build/check with `scripts/build-client.ps1` and `python tests/check-site.py`; use the deployment command from WSL. GitHub Actions uses read-only repository permissions, pinned actions, a production environment and a dedicated non-privileged key. Do not add a public-repository runner to this shared server or another application's account.

## Rollback

Use the dedicated SSH account and the release id printed by the workflow:

```bash
ssh codaris-rbx 'readlink /srv/codaris/current; readlink /srv/codaris/previous'
ssh codaris-rbx 'sudo -n /usr/local/sbin/codaris-deploy-release rollback RELEASE_ID'
```

Rollback restores the preceding site and API plus the CODARIS Nginx files saved for that release. It restarts only `codaris-api.service`, checks local API health, validates the full Nginx configuration, and reloads shared Nginx only if validation succeeds. Database state is never rolled back by the release helper. Investigate before deleting releases or backups.

## Public verification

Check HTTPS certificate/renewal, canonical redirects, all routes, country search, globe interaction and missing-page 404s. Confirm `Content-Security-Policy`, `X-Content-Type-Options`, `X-Frame-Options`, HSTS, and correct `application/wasm` MIME type. Confirm `.git`, `.env`, source/config files and unsupported methods are inaccessible. Check the browser console for unexpected CSP violations. The policy permits Wasm compilation without JavaScript `unsafe-eval`; inline executable code, HTML script sinks, third-party scripts and form submission are blocked. This reduces attack surface; it does not guarantee immunity to XSS or network attacks.

## API process lifecycle

The repository already contains the long-running native C API and the one-shot mail worker. Run them as the dedicated `codaris` operating-system account using `deploy/codaris-api.service` and `deploy/codaris-mail.service` plus `deploy/codaris-mail.timer`. The API binds only to `127.0.0.1`; Nginx terminates HTTPS on `api.codaris.org` and proxies `/api/` to it. The public-site vhost uses a separate internal location for its member-page access check. The service units use systemd restart behavior and sandboxing. `scripts/dev.sh` is only a local-development launcher; systemd starts production services at boot and keeps them running. PM2 is not needed and must not be shared with Coupyn, ChatPDM or other applications.

The API binary and credential font are included in each static site release package and kept in versioned directories under `/opt/codaris/releases`. The private backend environment, PostgreSQL data and schema are not packaged. Before a release that depends on a schema change, take and verify a database backup, apply the reviewed migration manually, then merge/deploy the code. Keep schema changes backward-compatible with the previous API binary until its rollback window has passed. The mail timer runs `codaris-mail.service` once per minute. Check `systemctl status codaris-api.service codaris-mail.timer`, `journalctl -u codaris-api.service`, and `https://api.codaris.org/api/health` after each backend release. Never enable account registration until API health reports `page_access_api: 1` and the browser-facing account flows have passed.

## Compression and availability limits

Nginx gzip is enabled for HTML and public static CSS, JavaScript, WebAssembly, SVG and XML. Personalized JSON API responses are deliberately not in `gzip_types`; compressing responses that combine secrets with attacker-controlled data can create side-channel risks. Gzip is built into normal Nginx packages, so CODARIS does not require a third-party Brotli module. Consider Brotli only if the installed Nginx package already provides a maintained module and measurements show a material benefit. Test both transfer size and CPU under representative traffic before changing compression settings.

The API location applies a per-client request rate and a concurrent-connection cap. These reduce simple floods and protect the single-threaded API, but IP-based limits affect people sharing a NAT and do not stop distributed denial-of-service traffic. Keep the host provider's network-level DDoS protection and firewall in place; do not change shared-host firewall rules for CODARIS without assessing every hosted application. A public availability monitor should check `https://api.codaris.org/api/health`, certificate expiry and mail-worker failures without submitting real registrations.

The included limits are starting safeguards, not capacity guarantees. Load-test the expected traffic and tune them based on observed legitimate bursts, PostgreSQL capacity and Argon2id cost. Keep test traffic away from the live account database.

References: [Nginx header inheritance](https://nginx.org/en/docs/http/ngx_http_headers_module.html), [Nginx trusted real-IP module](https://nginx.org/en/docs/http/ngx_http_realip_module.html), [Cloudflare visitor-IP restoration and trusted ranges](https://developers.cloudflare.com/support/troubleshooting/restoring-visitor-ips/restoring-original-visitor-ips/), [CSP script policy](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Content-Security-Policy/script-src), [Emscripten settings](https://emscripten.org/docs/tools_reference/settings_reference.html#dynamic-execution), [GitHub Actions security](https://docs.github.com/en/actions/reference/security/secure-use).
