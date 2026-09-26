#!/usr/bin/env bash
set -euo pipefail

# Open a loopback-only tunnel for desktop database tools. This is deliberately
# separate from dev.sh, which uses the isolated local development database.
SSH_HOST="${CODARIS_DB_SSH_HOST:-100.83.44.53}"
SSH_USER="${CODARIS_DB_SSH_USER:-coupynops}"
LOCAL_PORT="${CODARIS_DB_LOCAL_PORT:-15432}"

if [[ -n "${CODARIS_DB_TUNNEL_KEY:-}" ]]; then
    KEY="$CODARIS_DB_TUNNEL_KEY"
else
    KEY="$HOME/.ssh/codaris_db_read"
    if [[ ! -r "$KEY" ]] && command -v cmd.exe >/dev/null 2>&1 && command -v wslpath >/dev/null 2>&1; then
        WINDOWS_PROFILE="$(cmd.exe /C 'echo %USERPROFILE%' 2>/dev/null | tr -d '\r')"
        if [[ -n "$WINDOWS_PROFILE" ]]; then
            KEY="$(wslpath -u "$WINDOWS_PROFILE")/.ssh/codaris_db_read"
        fi
    fi
fi

if [[ ! -r "$KEY" ]]; then
    echo "CODARIS tunnel key is not readable: $KEY" >&2
    echo 'Set CODARIS_DB_TUNNEL_KEY to the private-key path, or place it at ~/.ssh/codaris_db_read.' >&2
    exit 1
fi
if [[ ! "$LOCAL_PORT" =~ ^[0-9]+$ ]] || (( LOCAL_PORT < 1024 || LOCAL_PORT > 65535 )); then
    echo 'CODARIS_DB_LOCAL_PORT must be a port between 1024 and 65535.' >&2
    exit 1
fi
if ! command -v ssh >/dev/null 2>&1; then
    echo 'ssh is required to open the CODARIS database tunnel.' >&2
    exit 1
fi

printf 'Forwarding localhost:%s to the CODARIS PostgreSQL server. Keep this terminal open; press Ctrl+C to close it.\n' "$LOCAL_PORT"
exec ssh \
    -N \
    -o ExitOnForwardFailure=yes \
    -o ForwardAgent=no \
    -o ServerAliveInterval=30 \
    -o ServerAliveCountMax=3 \
    -i "$KEY" \
    -L "127.0.0.1:${LOCAL_PORT}:127.0.0.1:5432" \
    "${SSH_USER}@${SSH_HOST}"
