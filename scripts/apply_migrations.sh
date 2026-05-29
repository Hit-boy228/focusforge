#!/bin/bash
# scripts/apply_migrations.sh
set -e

source "$(dirname "$0")/wait_for_services.sh" 30

echo "🗄️  Applying PostgreSQL migrations..."
for f in migrations/postgres/*.sql; do
    echo "  → $f"
    PGPASSWORD="$POSTGRES_PASSWORD" psql \
        -h "${POSTGRES_HOST:-localhost}" \
        -U "${POSTGRES_USER:-focusforge}" \
        -d "${POSTGRES_DB:-focusforge}" \
        -f "$f"
done

echo "🍃 Applying MongoDB migrations..."
for f in migrations/mongo/*.js; do
    echo "  → $f"
    mongosh --host "${MONGO_HOST:-localhost}" \
            --username "${MONGO_USER:-focusforge}" \
            --password "${MONGO_PASSWORD}" \
            --authenticationDatabase admin \
            "$f"
done

echo "✅ All migrations applied!"
