-- =============================================================================
-- Migration 002: Additional Indexes
-- =============================================================================

BEGIN;

-- ── Составные индексы для аналитических запросов ─────────────────────────────

-- Продуктивность по часам: сессии пользователя с группировкой по часу
CREATE INDEX IF NOT EXISTS idx_sessions_user_hour
    ON focus_sessions (user_id, EXTRACT(hour FROM started_at))
    WHERE status = 'completed';

-- Задачи по дням (для дневной статистики)
CREATE INDEX IF NOT EXISTS idx_tasks_completed_date
    ON tasks (user_id, completed_at::date)
    WHERE status = 'done' AND NOT is_deleted;

-- Теги пользователя для поиска по тегу
CREATE INDEX IF NOT EXISTS idx_tags_user_name
    ON tags (user_id, lower(name));

-- Напоминания по задаче (для каскадного удаления / просмотра)
CREATE INDEX IF NOT EXISTS idx_reminders_task
    ON reminders (task_id)
    WHERE task_id IS NOT NULL;

-- Активные цели пользователя в текущем периоде
CREATE INDEX IF NOT EXISTS idx_goals_user_active_period
    ON goals (user_id, type, period_end)
    WHERE is_active = TRUE;

-- Activity log для дашборда: последние события пользователя
CREATE INDEX IF NOT EXISTS idx_activity_user_recent
    ON activity_log (user_id, occurred_at DESC);

-- Idempotency keys: очистка просроченных
CREATE INDEX IF NOT EXISTS idx_idempotency_expires_at
    ON idempotency_keys (expires_at)
    WHERE expires_at < NOW();

COMMIT;
