#!/bin/sh
set -e

TOKEN="${TELEGRAM_BOT_TOKEN}"
SECRET="${TELEGRAM_WEBHOOK_SECRET}"
TARGET="http://focusforge:8080"
LOG="/tmp/cf.log"

register_webhook() {
    local tunnel_url="$1"
    local wh_url="${tunnel_url}/webhook/${TOKEN}"
    echo "[tunnel] Registering webhook: ${wh_url}"
    result=$(curl -sf -X POST "https://api.telegram.org/bot${TOKEN}/setWebhook" \
        -H "Content-Type: application/json" \
        -d "{\"url\":\"${wh_url}\",\"secret_token\":\"${SECRET}\"}" 2>&1)
    echo "[tunnel] Telegram response: ${result}"
}

while true; do
    echo "[tunnel] Starting cloudflared tunnel -> ${TARGET}"
    : > "${LOG}"

    cloudflared tunnel --url "${TARGET}" --no-autoupdate 2>&1 | tee "${LOG}" &
    CF_PID=$!

    # Wait up to 60s for the trycloudflare.com URL to appear in output
    TUNNEL_URL=""
    i=0
    while [ $i -lt 30 ]; do
        TUNNEL_URL=$(grep -oE 'https://[a-zA-Z0-9-]+\.trycloudflare\.com' "${LOG}" 2>/dev/null | head -1)
        [ -n "${TUNNEL_URL}" ] && break
        sleep 2
        i=$((i + 1))
    done

    if [ -z "${TUNNEL_URL}" ]; then
        echo "[tunnel] Timed out waiting for tunnel URL — restarting in 10s..."
        kill "${CF_PID}" 2>/dev/null || true
        wait "${CF_PID}" 2>/dev/null || true
        sleep 10
        continue
    fi

    echo "[tunnel] Got URL: ${TUNNEL_URL}"
    register_webhook "${TUNNEL_URL}"

    # Block until cloudflared exits (crash or SIGTERM)
    wait "${CF_PID}" || true
    echo "[tunnel] Tunnel process exited — restarting in 5s..."
    sleep 5
done
