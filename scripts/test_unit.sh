#!/bin/bash
# scripts/test_unit.sh
set -e

BUILD_DIR="${BUILD_DIR:-build}"
echo "🧪 Running unit tests..."

cmake --build "$BUILD_DIR" --target focusforge_unit_tests -j$(nproc)
"$BUILD_DIR/tests/focusforge_unit_tests" \
    --gtest_output="xml:test_results_unit.xml" \
    "$@"

echo "✅ Unit tests passed!"
