#!/bin/sh
# deploy/migrator/run_migrations.sh
# Последовательно применяет PostgreSQL миграции

set -e

echo "Waiting for PostgreSQL..."
until pg_isready -h "$PGHOST" -p "$PGPORT" -U "$PGUSER"; do
    sleep 1
done

echo "PostgreSQL is ready. Applying migrations..."

for migration in /migrations/postgres/*.sql; do
    echo "Applying: $migration"
    psql -f "$migration" || {
        echo "ERROR: Migration failed: $migration"
        exit 1
    }
done

echo "All migrations applied successfully."
