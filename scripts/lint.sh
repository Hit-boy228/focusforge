#!/bin/bash
# scripts/lint.sh
set -e
echo "🔍 Running clang-tidy..."

BUILD_DIR="${BUILD_DIR:-build}"
if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "❌ compile_commands.json not found. Run cmake first."
    exit 1
fi

find src -name "*.cpp" -o -name "*.hpp" | \
    xargs clang-tidy -p "$BUILD_DIR" --warnings-as-errors='*'

echo "✅ Lint passed!"
