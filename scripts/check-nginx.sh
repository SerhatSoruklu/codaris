#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
NGINX="$(command -v nginx || true)"
[[ -n "$NGINX" ]] || { echo 'nginx is required for configuration validation.' >&2; exit 1; }
for file in "$ROOT/build/deploy/security-headers.conf" "$ROOT/build/deploy/member-routes.conf"; do
  [[ -f "$file" ]] || { echo 'Build the production client before validating Nginx.' >&2; exit 1; }
done
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/site"
openssl req -x509 -nodes -newkey rsa:2048 -days 1 -subj '/CN=codaris.invalid' \
  -keyout "$TMP/key.pem" -out "$TMP/cert.pem" >/dev/null 2>&1
cat >"$TMP/cloudflare-real-ip.conf" <<'EOF'
set_real_ip_from 127.0.0.1;
real_ip_header CF-Connecting-IP;
real_ip_recursive on;
EOF
sed \
  -e "s|/etc/nginx/snippets/codaris-cloudflare-real-ip.conf|$TMP/cloudflare-real-ip.conf|g" \
  -e "s|/etc/nginx/snippets/codaris-security-headers.conf|$ROOT/build/deploy/security-headers.conf|g" \
  -e "s|/etc/nginx/snippets/codaris-member-routes.conf|$ROOT/build/deploy/member-routes.conf|g" \
  -e "s|/etc/letsencrypt/live/codaris.org/fullchain.pem|$TMP/cert.pem|g" \
  -e "s|/etc/letsencrypt/live/codaris.org/privkey.pem|$TMP/key.pem|g" \
  -e "s|/etc/letsencrypt/live/api.codaris.org/fullchain.pem|$TMP/cert.pem|g" \
  -e "s|/etc/letsencrypt/live/api.codaris.org/privkey.pem|$TMP/key.pem|g" \
  -e "s|/var/log/nginx/codaris.access.log|$TMP/codaris.access.log|g" \
  -e "s|/var/log/nginx/codaris.error.log|$TMP/codaris.error.log|g" \
  -e "s|/srv/codaris/current|$TMP/site|g" \
  -e 's/listen 80;/listen 18080;/g' \
  -e 's/listen \[::\]:80;/listen [::]:18080;/g' \
  -e 's/listen 443 ssl;/listen 18443 ssl;/g' \
  -e 's/listen \[::\]:443 ssl;/listen [::]:18443 ssl;/g' \
  "$ROOT/deploy/nginx/codaris.conf" >"$TMP/codaris.conf"
cat >"$TMP/nginx.conf" <<EOF
worker_processes 1;
pid $TMP/nginx.pid;
error_log stderr;
events { worker_connections 16; }
http {
    include /etc/nginx/mime.types;
    access_log $TMP/access.log;
    include $TMP/codaris.conf;
}
EOF
"$NGINX" -t -p "$TMP/" -c "$TMP/nginx.conf"
