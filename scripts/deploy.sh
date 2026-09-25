#!/usr/bin/env bash
# Run from Linux/WSL after an administrator provisions the isolated account.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
TARGET="${1:?Usage: ./scripts/deploy.sh <dedicated-codaris-ssh-alias>}"
[[ "$TARGET" =~ ^[A-Za-z0-9][A-Za-z0-9_.-]*$ ]] || { echo 'Use a configured SSH alias.' >&2; exit 1; }
[[ -z "$(git status --porcelain)" ]] || { echo 'Commit or resolve local changes before deploying.' >&2; exit 1; }
REVISION="$(git rev-parse HEAD)"
# Do not publish a local revision that has not reached the protected main branch.
git fetch origin main
[[ "$REVISION" == "$(git rev-parse origin/main)" ]] || { echo 'Deploy the current origin/main revision.' >&2; exit 1; }
RELEASE="$(date -u +%Y%m%dT%H%M%SZ)-${REVISION:0:12}"
export CODARIS_PRODUCTION=1
python3 "$ROOT/scripts/project.py" production build-client
python3 "$ROOT/tests/check-site.py"
printf '%s\n' "$REVISION" > "$ROOT/build/client/version.txt"
# Package only reviewed public files, never repository contents or backend secrets.
python3 "$ROOT/scripts/package-site.py" "$RELEASE"
SSH_ARGS=(-o BatchMode=yes -o StrictHostKeyChecking=yes -o ForwardAgent=no -o ClearAllForwardings=yes)
ssh "${SSH_ARGS[@]}" "$TARGET" "test \"\$(id -un)\" = codaris-deploy && test -d /srv/codaris/releases && mkdir /srv/codaris/releases/$RELEASE"
rsync -rlt --chmod=D750,F640 -e 'ssh -o BatchMode=yes -o StrictHostKeyChecking=yes -o ForwardAgent=no -o ClearAllForwardings=yes' "$ROOT/build/package/$RELEASE/" "$TARGET:/srv/codaris/releases/$RELEASE/"
ssh "${SSH_ARGS[@]}" "$TARGET" bash -s -- "$RELEASE" <<'REMOTE'
set -euo pipefail
release="$1"
cd "/srv/codaris/releases/$release"
sha256sum --check SHA256SUMS
# Keep the manifest outside the public root; activate only the site directory.
previous="$(readlink /srv/codaris/current || true)"
ln -s "/srv/codaris/releases/$release/site" "/srv/codaris/.current-$release"
mv -Tf "/srv/codaris/.current-$release" /srv/codaris/current
if [[ -n "$previous" ]]; then
    ln -s "$previous" "/srv/codaris/.previous-$release"
    mv -Tf "/srv/codaris/.previous-$release" /srv/codaris/previous
fi
printf 'Activated %s\n' "$release"
REMOTE
if ! LIVE_REVISION="$(curl --fail --silent --show-error --max-time 20 https://codaris.org/version.txt)" || [[ "$LIVE_REVISION" != "$REVISION" ]]; then
    echo 'Public health check failed. Investigate and use the documented rollback command.' >&2
    exit 1
fi
printf 'Deployed %s. Verify TLS, headers and the interactive pages before announcing.\n' "$REVISION"
