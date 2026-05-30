#!/bin/bash
# scripts/format.sh
# Использование: ./scripts/format.sh [--check]
# Переопределить бинарь: CLANG_FORMAT=clang-format-18 ./scripts/format.sh --check
set -e

CF="${CLANG_FORMAT:-clang-format}"
echo "🎨 Formatting code with ${CF}..."

FILES=$(find src tests -name "*.cpp" -o -name "*.hpp" 2>/dev/null)
if [ -z "$FILES" ]; then
    echo "No source files found."
    exit 0
fi

if [ "${1}" == "--check" ]; then
    echo "$FILES" | xargs "$CF" --dry-run --Werror
    echo "✅ Format check passed!"
else
    echo "$FILES" | xargs "$CF" -i
    echo "✅ Code formatted!"
fi
