# Production deployment safeguards

This guide explains what the CODARIS GitHub Actions deployment protects, what it does not check, and how to recover from a failed release. The workflow is defined in `.github/workflows/verify.yml`; the release receiver and rollback logic live in `deploy/codaris-deploy-release`.

## Normal change path

1. Make a small change on a branch and open a pull request into `main`.
2. Wait for **Build and security checks** to pass. The workflow builds the browser client and native C API, checks generated pages and public artifacts, validates Nginx syntax, creates an allowlisted release package, and checks its checksums and deployment helper syntax.
3. Review the change and merge the pull request. The `main` push starts the production deploy job when the repository variable `CODARIS_DEPLOY_ENABLED` is `true`.
4. Check the **Deploy CODARIS production** job. It must finish green before treating the release as live.

GitHub branch protection is configured outside this repository. Confirm that `main` requires a pull request and the `Build and security checks` result, and blocks force pushes and branch deletion. The workflow itself builds pushes to `main`; it cannot prevent a direct push if GitHub branch rules are removed or misconfigured. See [deployment setup](DEPLOYMENT.md#github-actions-production-deployment).

## What the deployment checks

- The deploy job only runs for `main` pushes or manual workflow runs on `main`, after the build job succeeds. Production deployments run one at a time.
- GitHub uses a dedicated deploy key, a pinned SSH host key, and a temporary Tailscale runner. The key connects as `codaris-deploy`; it is not a root key and does not provide a general sudo shell.
- The workflow sends the verified build artifact. The server accepts only allowlisted site, API, and CODARIS Nginx files, rejects duplicate or unexpected archive members, and checks the SHA-256 manifest.
- The server checks `nginx -t`, points the API service at the candidate release, restarts only `codaris-api.service`, and waits for local API health before switching the site pointer. It reloads shared Nginx only after valid configuration and only when the CODARIS configuration changed.
- After activation, the workflow checks that the public site serves the expected commit, required security headers are present, and `https://api.codaris.org/api/health` reports the required API versions.
- If server activation fails, the helper restores the previous site, API, and CODARIS Nginx configuration. If public verification fails, the deploy script requests that same guarded rollback.

Deployments do **not** apply database migrations. They also do not exercise complete registration, login, email, credential, or account deletion flows against production. A green workflow verifies the build, configured checks, and basic public availability; it is not a guarantee that every user journey or third-party service works.

## If a pull request check fails

Do not merge while a required check is red. Open the failed **Build and security checks** job, expand the first failing step, and fix the reported source or workflow issue on the branch. Push the correction and wait for the PR checks to finish again. If the change is no longer needed, close the PR; no production release has run from a PR check.

## If deployment fails

Open the failed deploy job and locate the first failed step.

- If failure is in the Tailscale, SSH, artifact, or host-key setup, the server activation step was not reached. Resolve the named GitHub environment setting, secret, tailnet path, or artifact issue, then rerun the failed workflow after confirming the cause.
- If failure occurs during activation, the server helper attempts to restore the previous site/API/config. If public verification fails after activation, the workflow requests the guarded rollback. Confirm the rollback result in the log before retrying.
- If the job reports that automatic rollback failed, stop repeated deploy attempts and ask the server administrator to inspect service state and logs. Use the manual recovery steps below.

For a release that reached the server, the deploy log prints its release identifier. The `codaris-deploy` account may invoke only the CODARIS release helper; it cannot read systemd status or journal logs through sudo. In an administrator SSH session on the server, inspect the active and previous site pointers and relevant service state:

```bash
sudo readlink /srv/codaris/current
sudo readlink /srv/codaris/previous
sudo systemctl status codaris-api.service nginx --no-pager
sudo journalctl -u codaris-api.service -n 100 --no-pager
curl --fail --show-error https://codaris.org/version.txt
curl --fail --show-error https://api.codaris.org/api/health
```

If the current release is bad and its previous release is known healthy, roll back the release id shown in the workflow log:

```bash
sudo /usr/local/sbin/codaris-deploy-release rollback RELEASE_ID
```

The helper only permits rollback of the currently active release. It restores the saved CODARIS Nginx files and preceding site/API, validates Nginx, restarts the CODARIS API, checks local API health, and reloads Nginx if valid. Verify the public version and health endpoints again afterward. A rollback changes the live release, not Git history: fix the source in a pull request so the next deployment does not reintroduce the issue.

## Database and shared-server limits

The release rollback does not restore PostgreSQL data or reverse a schema migration. Before any schema change, take and verify a database backup, review the migration, and keep it compatible with both the new and previous API release. Apply migrations as a separate administrator operation; do not add them to the automatic site deploy without a reviewed migration/rollback design.

The host runs other applications and shares one Nginx process. The deployment helper only changes CODARIS files and reloads Nginx after validation, but a server-wide hardware, network, OS, Nginx, or database failure is outside the GitHub release rollback. Avoid changing shared firewall rules, services, or Nginx configuration outside the CODARIS files without checking effects on the other hosted applications.

Old site/API releases and root-owned Nginx backups are retained for explicit rollback and cleanup. Do not delete the current or previous release or its backup while investigating an incident.
