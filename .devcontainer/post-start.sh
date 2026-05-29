#!/bin/bash
# .devcontainer/post-start.sh
# Выполняется при КАЖДОМ старте Dev Container

set -e
cd /workspace

echo "▶ FocusForge Dev Container started"

# Ждём готовности сервисов (не более 30 сек)
./scripts/wait_for_services.sh 30 2>/dev/null || \
    echo "⚠  Some services may not be ready yet"

# Применяем миграции (idempotent — безопасно запускать повторно)
echo "▶ Applying migrations..."
./scripts/apply_migrations.sh 2>/dev/null && echo "✓ Migrations applied" || \
    echo "⚠  Could not apply migrations (DB may not be ready yet)"

# Обновляем симлинк compile_commands если build существует
[ -f build/compile_commands.json ] && \
    ln -sf build/compile_commands.json compile_commands.json 2>/dev/null || true

echo "✓ Dev container ready"
