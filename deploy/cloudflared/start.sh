#!/bin/sh
# Downloads arm64 cloudflared (cached) and starts the tunnel.
# Exponential backoff on failure to avoid rate limiting.

set -e

CACHE="/cache/cloudflared"
TARGET="http://focusforge:8080"
RETRY=0

if [ ! -f "$CACHE" ]; then
    echo "[cloudflared] Downloading cloudflared arm64..."
    apk add --no-cache curl >/dev/null 2>&1
    curl -fsSL -o "$CACHE" \
        https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64
    chmod +x "$CACHE"
    echo "[cloudflared] Ready: $($CACHE --version)"
fi

while true; do
    echo "[cloudflared] Starting tunnel -> $TARGET (attempt $((RETRY + 1)))"
    "$CACHE" tunnel --url "$TARGET" --no-autoupdate 2>&1 | tee /log/cloudflared.log
    EXIT=$?
    RETRY=$((RETRY + 1))

    # Exponential backoff: 10s, 20s, 40s, 80s, max 120s
    WAIT=$((10 * (1 << RETRY > 12 ? 12 : RETRY)))
    [ $WAIT -gt 120 ] && WAIT=120
    echo "[cloudflared] Tunnel exited (code=$EXIT). Retry in ${WAIT}s..."
    sleep $WAIT
done
