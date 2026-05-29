#!/bin/bash
# .devcontainer/post-create.sh
# Выполняется ОДИН РАЗ при первом создании Dev Container

set -e
cd /workspace

echo "╔══════════════════════════════════════════════════════════╗"
echo "║         FocusForge Dev Container — First Setup           ║"
echo "╚══════════════════════════════════════════════════════════╝"

# ── 1. Копируем .env если нет ─────────────────────────────────────────────────
if [ ! -f .env ]; then
    cp .env.example .env
    echo "✓ Created .env from .env.example"
fi

# ── 2. Conan profile ──────────────────────────────────────────────────────────
conan profile detect --force 2>/dev/null || true
echo "✓ Conan profile ready"

# ── 3. Настраиваем CMake (Debug для разработки) ───────────────────────────────
echo "▶ Configuring CMake (Debug)..."
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DENABLE_INTEGRATION_TESTS=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -GNinja \
    2>&1 | tail -5

# Симлинк compile_commands.json для clangd
ln -sf build/compile_commands.json compile_commands.json 2>/dev/null || true
echo "✓ CMake configured"

# ── 4. Python-зависимости для тестов ─────────────────────────────────────────
pip3 install httpx pytest pytest-asyncio --quiet
echo "✓ Python test dependencies installed"

# ── 5. Git hooks ─────────────────────────────────────────────────────────────
if [ -d .git ]; then
    cat > .git/hooks/pre-commit << 'HOOK'
#!/bin/bash
echo "▶ Running format check..."
make format-check || {
    echo "❌ Format check failed. Run: make format"
    exit 1
}
echo "✔ Format OK"
HOOK
    chmod +x .git/hooks/pre-commit
    echo "✓ Git pre-commit hook installed"
fi

# ── 6. Директория test-results ────────────────────────────────────────────────
mkdir -p test-results

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║                  Setup complete! 🚀                      ║"
echo "╠══════════════════════════════════════════════════════════╣"
echo "║  make build        — собрать проект                      ║"
echo "║  make migrate      — применить миграции                  ║"
echo "║  make run          — запустить приложение                ║"
echo "║  make test         — запустить тесты                     ║"
echo "║  make help         — все команды                         ║"
echo "╚══════════════════════════════════════════════════════════╝"
