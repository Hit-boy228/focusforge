#!/bin/bash
# scripts/seed_demo_data.sh
set -e
echo "🌱 Seeding demo data..."

PGPASSWORD="$POSTGRES_PASSWORD" psql \
    -h "${POSTGRES_HOST:-localhost}" \
    -U "${POSTGRES_USER:-focusforge}" \
    -d "${POSTGRES_DB:-focusforge}" \
    -f tests/fixtures/postgres/seed.sql

mongosh --host "${MONGO_HOST:-localhost}" \
        --username "${MONGO_USER:-focusforge}" \
        --password "${MONGO_PASSWORD}" \
        --authenticationDatabase admin \
        tests/fixtures/mongo/seed.js

echo "✅ Demo data seeded!"
