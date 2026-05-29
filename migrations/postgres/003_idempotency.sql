-- =============================================================================
-- Migration 003: Idempotency & Telegram Update Deduplication
-- =============================================================================

BEGIN;

-- Таблица для дедупликации Telegram updates
-- update_id от Telegram уникален в рамках бота
CREATE TABLE IF NOT EXISTS telegram_processed_updates (
    update_id   BIGINT PRIMARY KEY,
    processed_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Удаляем старые записи (старше 7 дней) через TTL-like индекс
-- Очистка выполняется периодическим заданием
CREATE INDEX idx_telegram_updates_processed_at
    ON telegram_processed_updates (processed_at);

-- Расширяем idempotency_keys: добавляем user_id для шардинга и аудита
ALTER TABLE idempotency_keys
    ADD COLUMN IF NOT EXISTS user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    ADD COLUMN IF NOT EXISTS operation VARCHAR(128);

CREATE INDEX idx_idempotency_user ON idempotency_keys(user_id)
    WHERE user_id IS NOT NULL;

-- Функция для автоматической очистки просроченных idempotency keys
CREATE OR REPLACE FUNCTION cleanup_expired_idempotency_keys()
RETURNS void AS $$
BEGIN
    DELETE FROM idempotency_keys WHERE expires_at < NOW();
    DELETE FROM telegram_processed_updates
        WHERE processed_at < NOW() - INTERVAL '7 days';
END;
$$ LANGUAGE plpgsql;

COMMIT;
