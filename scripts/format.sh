#!/bin/bash
# scripts/format.sh
set -e
echo "🎨 Formatting code with clang-format..."

if [ "${1}" == "--check" ]; then
    find src tests -name "*.cpp" -o -name "*.hpp" | \
        xargs clang-format --dry-run --Werror
    echo "✅ Format check passed!"
else
    find src tests -name "*.cpp" -o -name "*.hpp" | \
        xargs clang-format -i
    echo "✅ Code formatted!"
fi
