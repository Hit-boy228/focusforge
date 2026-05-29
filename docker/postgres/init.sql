-- docker/postgres/init.sql
-- Выполняется при первом старте контейнера postgres

-- Создаём базу данных (уже создана через POSTGRES_DB env)
-- Создаём extension для UUID
\c focusforge;

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pg_trgm";
CREATE EXTENSION IF NOT EXISTS "pg_stat_statements";

-- Настройка параметров для разработки
ALTER SYSTEM SET log_min_duration_statement = '100';  -- логируем запросы > 100ms
ALTER SYSTEM SET log_statement = 'none';
ALTER SYSTEM SET track_counts = on;
ALTER SYSTEM SET track_activities = on;

SELECT pg_reload_conf();
