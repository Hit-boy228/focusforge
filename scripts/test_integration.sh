#!/bin/bash
# scripts/test_integration.sh
set -e

echo "🧪 Running integration tests..."
./scripts/wait_for_services.sh 60
./scripts/apply_migrations.sh
./scripts/seed_demo_data.sh

BUILD_DIR="${BUILD_DIR:-build}"
cmake --build "$BUILD_DIR" --target focusforge_integration_tests -j$(nproc)

ENABLE_INTEGRATION_TESTS=1 \
POSTGRES_HOST="${POSTGRES_HOST:-localhost}" \
REDIS_HOST="${REDIS_HOST:-localhost}" \
MONGO_HOST="${MONGO_HOST:-localhost}" \
"$BUILD_DIR/tests/focusforge_integration_tests" \
    --gtest_output="xml:test_results_integration.xml" "$@"

echo "🐍 Running functional tests..."
pip install httpx pytest -q
pytest tests/functional/ -v --tb=short

echo "✅ All integration tests passed!"
