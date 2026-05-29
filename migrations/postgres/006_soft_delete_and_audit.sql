-- =============================================================================
-- Migration 006: Soft Delete Policy, Audit Trail, Snooze
-- =============================================================================

BEGIN;

-- ── Snooze history для напоминаний ───────────────────────────────────────────
CREATE TABLE reminder_snoozes (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    reminder_id     UUID NOT NULL REFERENCES reminders(id) ON DELETE CASCADE,
    user_id         UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    snoozed_until   TIMESTAMPTZ NOT NULL,
    reason          VARCHAR(128),   -- "занят", "жду ответ", "нет энергии"
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_reminder_snoozes_reminder ON reminder_snoozes(reminder_id);
CREATE INDEX idx_reminder_snoozes_user ON reminder_snoozes(user_id, created_at DESC);

-- ── Timeboxing / Calendar Blocks ─────────────────────────────────────────────
CREATE TABLE time_blocks (
    id          UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id     UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    task_id     UUID REFERENCES tasks(id) ON DELETE SET NULL,
    title       VARCHAR(256) NOT NULL,
    starts_at   TIMESTAMPTZ NOT NULL,
    ends_at     TIMESTAMPTZ NOT NULL,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),

    CONSTRAINT chk_time_block_order CHECK (ends_at > starts_at),
    CONSTRAINT chk_time_block_duration
        CHECK (EXTRACT(EPOCH FROM (ends_at - starts_at)) / 60 BETWEEN 5 AND 480)
);

CREATE INDEX idx_time_blocks_user_date ON time_blocks(user_id, starts_at);
CREATE INDEX idx_time_blocks_task ON time_blocks(task_id)
    WHERE task_id IS NOT NULL;

-- ── Inbox (Quick Capture) ────────────────────────────────────────────────────
CREATE TABLE inbox_items (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    raw_text        TEXT NOT NULL,
    parsed_title    VARCHAR(512),
    parsed_deadline TIMESTAMPTZ,
    parsed_priority task_priority,
    parsed_tags     TEXT[],
    is_processed    BOOLEAN NOT NULL DEFAULT FALSE,
    task_id         UUID REFERENCES tasks(id) ON DELETE SET NULL,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    processed_at    TIMESTAMPTZ
);

CREATE INDEX idx_inbox_user_unprocessed ON inbox_items(user_id, created_at)
    WHERE NOT is_processed;

-- ── Streak tracking ──────────────────────────────────────────────────────────
CREATE TABLE user_streaks (
    user_id             UUID PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    current_streak      INTEGER NOT NULL DEFAULT 0,
    longest_streak      INTEGER NOT NULL DEFAULT 0,
    last_active_date    DATE,
    grace_days_used     INTEGER NOT NULL DEFAULT 0,
    grace_days_total    INTEGER NOT NULL DEFAULT 1,   -- 1 grace day по умолчанию
    streak_frozen_until DATE,
    updated_at          TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- ── Focus debt tracking ──────────────────────────────────────────────────────
-- Фиксируем "прерванные" сессии для recovery mode
ALTER TABLE focus_sessions
    ADD COLUMN IF NOT EXISTS interruption_count INTEGER NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS focus_debt_minutes INTEGER NOT NULL DEFAULT 0;

-- ── Soft-deleted tasks: view для восстановления ──────────────────────────────
CREATE OR REPLACE VIEW deleted_tasks AS
    SELECT * FROM tasks
    WHERE is_deleted = TRUE
    ORDER BY deleted_at DESC;

-- ── Политика очистки мягко удалённых задач (старше 30 дней) ─────────────────
CREATE OR REPLACE FUNCTION cleanup_old_soft_deleted_tasks()
RETURNS INTEGER AS $$
DECLARE
    deleted_count INTEGER;
BEGIN
    DELETE FROM tasks
    WHERE is_deleted = TRUE
      AND deleted_at < NOW() - INTERVAL '30 days'
    RETURNING count(*) INTO deleted_count;

    RETURN COALESCE(deleted_count, 0);
END;
$$ LANGUAGE plpgsql;

COMMIT;
