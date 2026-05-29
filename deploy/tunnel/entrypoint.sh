#!/bin/sh
# Открывает SSH-туннель через localhost.run (без аккаунта, без rate-limit).
# Извлекает HTTPS-URL и регистрирует Telegram webhook автоматически.
# При обрыве соединения — перезапускается и перерегистрирует новый URL.

set -e

TOKEN="${TELEGRAM_BOT_TOKEN}"
SECRET="${TELEGRAM_WEBHOOK_SECRET}"
TARGET="focusforge:8080"
LOG="/tmp/tunnel.log"

register() {
    local url="$1"
    local wh="${url}/webhook/${TOKEN}"
    echo "[tunnel] setWebhook → ${wh}"
    result=$(curl -sf -X POST "https://api.telegram.org/bot${TOKEN}/setWebhook" \
        -H "Content-Type: application/json" \
        -d "{\"url\":\"${wh}\",\"secret_token\":\"${SECRET}\",\"allowed_updates\":[\"message\",\"callback_query\",\"inline_query\"]}" 2>&1)
    echo "[tunnel] Telegram: ${result}"
}

while true; do
    echo "[tunnel] Connecting localhost.run → http://${TARGET}"
    : > "${LOG}"

    ssh \
        -o StrictHostKeyChecking=no \
        -o BatchMode=yes \
        -o ServerAliveInterval=30 \
        -o ServerAliveCountMax=3 \
        -o ExitOnForwardFailure=yes \
        -R "80:${TARGET}" \
        nokey@localhost.run \
        >"${LOG}" 2>&1 &
    SSH_PID=$!

    TUNNEL_URL=""
    for i in $(seq 1 20); do
        TUNNEL_URL=$(grep -oE 'https://[a-zA-Z0-9._-]+\.lhr\.life' "${LOG}" 2>/dev/null | head -1)
        [ -n "${TUNNEL_URL}" ] && break
        sleep 2
    done

    if [ -z "${TUNNEL_URL}" ]; then
        echo "[tunnel] URL not received in 40s — restarting in 15s..."
        kill "${SSH_PID}" 2>/dev/null || true
        wait "${SSH_PID}" 2>/dev/null || true
        sleep 15
        continue
    fi

    echo "[tunnel] URL: ${TUNNEL_URL}"
    register "${TUNNEL_URL}"

    # Блокируемся до падения SSH — после чего пойдём на новый цикл
    wait "${SSH_PID}" || true
    echo "[tunnel] Connection lost — restarting in 5s..."
    sleep 5
done
