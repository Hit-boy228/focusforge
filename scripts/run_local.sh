#!/bin/bash
# scripts/run_local.sh — единая команда старта локальной среды
set -e

echo "🚀 Starting FocusForge locally..."

# Копируем .env если его нет
if [ ! -f .env ]; then
    cp .env.example .env
    echo "📋 Created .env from .env.example — fill in your tokens!"
fi

docker compose up -d --build

echo "⏳ Waiting for services..."
sleep 5
./scripts/wait_for_services.sh 60

echo "🗄️  Applying migrations..."
./scripts/apply_migrations.sh

echo ""
echo "✅ FocusForge is running!"
echo "   App:     http://localhost:8080"
echo "   Health:  http://localhost:8080/health/live"
echo "   Adminer: http://localhost:8888"
echo ""
echo "📋 Register webhook:"
echo "   BOT_TOKEN=<your_token> ./scripts/register_webhook.sh"
