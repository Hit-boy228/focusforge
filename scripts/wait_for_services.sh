#!/bin/bash
# scripts/wait_for_services.sh
# Проверяет готовность сервисов используя CLI-инструменты доступные в dev-контейнере.
# Хосты берутся из переменных окружения (задаются в docker-compose.devcontainer.yml).
set -e

TIMEOUT=${1:-60}
POSTGRES_HOST=${POSTGRES_HOST:-localhost}
MONGO_HOST=${MONGO_HOST:-localhost}
REDIS_HOST=${REDIS_HOST:-localhost}
REDIS_PASSWORD=${REDIS_PASSWORD:-}

echo "⏳ Waiting for services (timeout: ${TIMEOUT}s)..."

wait_for() {
    local name=$1
    local cmd=$2
    local elapsed=0
    until eval "$cmd" &>/dev/null; do
        if [ $elapsed -ge $TIMEOUT ]; then
            echo "❌ Timeout waiting for $name"
            exit 1
        fi
        echo "  $name not ready, retrying..."
        sleep 2
        elapsed=$((elapsed + 2))
    done
    echo "  ✅ $name ready"
}

REDIS_AUTH=""
[ -n "$REDIS_PASSWORD" ] && REDIS_AUTH="-a $REDIS_PASSWORD"

wait_for "PostgreSQL" "pg_isready -h $POSTGRES_HOST -p 5432 -U focusforge"
wait_for "Redis"      "redis-cli -h $REDIS_HOST -p 6379 $REDIS_AUTH ping"
wait_for "MongoDB"    "mongosh --host $MONGO_HOST --port 27017 --eval 'db.runCommand({ping:1})' --quiet"

echo "✅ All services ready!"
