#!/bin/sh
# Reads cloudflared log, extracts tunnel URL, registers Telegram webhook.
# Re-checks every 5 min to catch URL changes after tunnel restarts.

TOKEN="${TELEGRAM_BOT_TOKEN}"
SECRET="${TELEGRAM_WEBHOOK_SECRET}"
LOG="/log/cloudflared.log"
LAST_URL=""

register() {
    local url="$1"
    local wh="${url}/webhook/${TOKEN}"
    echo "[registrar] setWebhook → ${wh}"
    result=$(curl -sf -X POST "https://api.telegram.org/bot${TOKEN}/setWebhook" \
        -H "Content-Type: application/json" \
        -d "{\"url\":\"${wh}\",\"secret_token\":\"${SECRET}\"}" 2>&1)
    echo "[registrar] Response: ${result}"
}

echo "[registrar] Waiting for cloudflared log at ${LOG}..."
while true; do
    if [ -f "${LOG}" ]; then
        URL=$(grep -oE 'https://[a-zA-Z0-9-]+\.trycloudflare\.com' "${LOG}" 2>/dev/null | tail -1)
        if [ -n "${URL}" ] && [ "${URL}" != "${LAST_URL}" ]; then
            LAST_URL="${URL}"
            echo "[registrar] New tunnel URL: ${URL}"
            register "${URL}"
        fi
    fi
    sleep 10
done
