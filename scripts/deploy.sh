#!/usr/bin/env bash
# Deploy an allowlisted release through the server's restricted CODARIS helper.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
TARGET="${1:?Usage: ./scripts/deploy.sh <dedicated-codaris-ssh-alias> [packaged-release]}"
PACKAGE_ID="${2:-}"
[[ "$TARGET" =~ ^[A-Za-z0-9][A-Za-z0-9_.-]*$ ]] || { echo 'Use a configured SSH alias.' >&2; exit 1; }
[[ -z "$(git status --porcelain)" ]] || { echo 'Commit or resolve local changes before deploying.' >&2; exit 1; }
REVISION="$(git rev-parse HEAD)"
[[ "$REVISION" =~ ^[0-9a-f]{40}$ ]] || { echo 'Could not determine the source revision.' >&2; exit 1; }
if [[ "${GITHUB_ACTIONS:-}" == true ]]; then
    [[ -n "$PACKAGE_ID" && "${GITHUB_REF:-}" == refs/heads/main && "${GITHUB_SHA:-}" == "$REVISION" ]] || {
        echo 'CI deployments must use the verified package for the main-branch commit.' >&2
        exit 1
    }
else
    git fetch origin main
    [[ "$REVISION" == "$(git rev-parse origin/main)" ]] || { echo 'Deploy only the current origin/main revision.' >&2; exit 1; }
fi
RELEASE="$(date -u +%Y%m%dT%H%M%SZ)-${REVISION:0:12}"

if [[ -n "$PACKAGE_ID" ]]; then
    [[ "$PACKAGE_ID" =~ ^[A-Za-z0-9][A-Za-z0-9_-]{0,79}$ ]] || { echo 'Invalid packaged release identifier.' >&2; exit 1; }
    PACKAGE_DIR="$ROOT/build/package/$PACKAGE_ID"
    [[ -d "$PACKAGE_DIR" && -f "$PACKAGE_DIR/SHA256SUMS" ]] || { echo 'The packaged release artifact is missing.' >&2; exit 1; }
    PACKAGE_REVISION="$(cat "$PACKAGE_DIR/site/version.txt")"
    [[ "$PACKAGE_REVISION" == "$REVISION" ]] || { echo 'The packaged commit does not match this checkout.' >&2; exit 1; }
    (cd "$PACKAGE_DIR" && sha256sum --check --status SHA256SUMS) || { echo 'The packaged release checksum failed.' >&2; exit 1; }
else
    export CODARIS_PRODUCTION=1
    python3 "$ROOT/scripts/project.py" production build-client
    SERVER_BUILD="$ROOT/build/server-release-system"
    cmake -S "$ROOT" -B "$SERVER_BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=/usr -DCMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH=FALSE \
        -DCMAKE_FIND_USE_PACKAGE_ROOT_PATH=FALSE
    cmake --build "$SERVER_BUILD" --parallel
    python3 "$ROOT/tests/check-site.py"
    "$ROOT/scripts/check-nginx.sh"
    printf '%s\n' "$REVISION" > "$ROOT/build/client/version.txt"
    CODARIS_API_BINARY="$SERVER_BUILD/codaris_server" python3 "$ROOT/scripts/package-site.py" "$RELEASE"
    PACKAGE_DIR="$ROOT/build/package/$RELEASE"
fi

SSH_ARGS=(-o BatchMode=yes -o StrictHostKeyChecking=yes -o ForwardAgent=no -o ClearAllForwardings=yes -o IdentitiesOnly=yes)
ssh "${SSH_ARGS[@]}" "$TARGET" 'test "$(id -un)" = codaris-deploy && test -d /srv/codaris/releases'
# A tar stream lets the root-owned receiver unpack into a private root directory;
# the SSH account cannot replace release files while root is activating them.
tar --sort=name --mtime='UTC 1970-01-01' --owner=0 --group=0 --numeric-owner \
    -C "$PACKAGE_DIR" -czf - . |
    ssh "${SSH_ARGS[@]}" -T "$TARGET" "sudo -n /usr/local/sbin/codaris-deploy-release activate $RELEASE"

verify_live_release() {
    local live_revision headers health
    live_revision="$(curl --fail --silent --show-error --max-time 20 "https://codaris.org/version.txt?release=$RELEASE")" || return 1
    [[ "$live_revision" == "$REVISION" ]] || { echo 'The public site is serving a different revision.' >&2; return 1; }
    headers="$(curl --fail --silent --show-error --head --max-time 20 "https://codaris.org/version.txt?release=$RELEASE")" || return 1
    for required_header in content-security-policy strict-transport-security x-content-type-options x-frame-options referrer-policy permissions-policy; do
        grep -qi "^${required_header}:" <<<"$headers" || {
            echo "The public response is missing $required_header." >&2
            return 1
        }
    done
    health="$(curl --fail --silent --show-error --max-time 20 https://api.codaris.org/api/health)" || {
        echo 'Public API /api/health did not return HTTP 200.' >&2
        return 1
    }
    printf '%s' "$health" | python3 -c 'import json,sys; d=json.load(sys.stdin); raise SystemExit(0 if d.get("contact_api")==1 and d.get("credential_api")==2 and d.get("page_access_api")==1 else 1)' || {
        echo 'Public /api/health does not report the required CODARIS API versions.' >&2
        return 1
    }
}

if ! verify_live_release; then
    echo 'Live verification failed; requesting rollback of this CODARIS release.' >&2
    if ! ssh "${SSH_ARGS[@]}" "$TARGET" "sudo -n /usr/local/sbin/codaris-deploy-release rollback $RELEASE"; then
        echo 'Automatic rollback failed. Use the documented administrator recovery steps.' >&2
    fi
    exit 1
fi
printf 'Deployed CODARIS revision %s as release %s; public site, headers and API health verified.\n' "$REVISION" "$RELEASE"
