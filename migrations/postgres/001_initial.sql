-- =============================================================================
-- Migration 001: Initial Schema
-- FocusForge — Complete PostgreSQL schema
-- =============================================================================

BEGIN;

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pg_trgm";

-- ── Enums ──────────────────────────────────────────────────────────────────────
CREATE TYPE task_status   AS ENUM ('new','in_progress','paused','done','archived');
CREATE TYPE task_priority AS ENUM ('low','medium','high','critical');
CREATE TYPE session_mode  AS ENUM ('pomodoro','deep_work','custom');
CREATE TYPE session_status AS ENUM ('active','paused','completed','cancelled');
CREATE TYPE goal_type     AS ENUM ('daily','weekly');
CREATE TYPE reminder_state AS ENUM ('pending','sent','snoozed','cancelled');
CREATE TYPE history_action AS ENUM (
    'created','updated','status_changed','priority_changed',
    'deadline_changed','deleted','archived','restored');

-- ── users ──────────────────────────────────────────────────────────────────────
CREATE TABLE users (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    telegram_id     BIGINT NOT NULL UNIQUE,
    username        VARCHAR(128),
    first_name      VARCHAR(256),
    last_name       VARCHAR(256),
    language_code   VARCHAR(10)  NOT NULL DEFAULT 'en',
    timezone        VARCHAR(64)  NOT NULL DEFAULT 'UTC',
    is_active       BOOLEAN      NOT NULL DEFAULT TRUE,

    -- Settings (denormalised for performance)
    daily_focus_goal_minutes    INTEGER NOT NULL DEFAULT 120,
    weekly_focus_goal_minutes   INTEGER NOT NULL DEFAULT 600,
    pomodoro_work_minutes       INTEGER NOT NULL DEFAULT 25,
    pomodoro_break_minutes      INTEGER NOT NULL DEFAULT 5,
    pomodoro_long_break_minutes INTEGER NOT NULL DEFAULT 15,
    deep_work_minutes           INTEGER NOT NULL DEFAULT 90,

    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_seen_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_users_telegram_id ON users(telegram_id);
CREATE INDEX idx_users_last_seen   ON users(last_seen_at DESC);

-- ── tags ───────────────────────────────────────────────────────────────────────
CREATE TABLE tags (
    id          UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id     UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name        VARCHAR(64) NOT NULL,
    color       VARCHAR(16) NOT NULL DEFAULT '#6B7280',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (user_id, name)
);

CREATE INDEX idx_tags_user ON tags(user_id);

-- ── tasks ──────────────────────────────────────────────────────────────────────
CREATE TABLE tasks (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    parent_task_id  UUID REFERENCES tasks(id) ON DELETE SET NULL,

    title           VARCHAR(512) NOT NULL,
    description     TEXT,
    status          task_status   NOT NULL DEFAULT 'new',
    priority        task_priority NOT NULL DEFAULT 'medium',

    deadline           TIMESTAMPTZ,
    estimated_minutes  INTEGER CHECK (estimated_minutes > 0),
    actual_minutes     INTEGER NOT NULL DEFAULT 0,

    is_recurring       BOOLEAN NOT NULL DEFAULT FALSE,
    recurrence_rule    VARCHAR(256),
    next_occurrence_at TIMESTAMPTZ,

    is_deleted    BOOLEAN     NOT NULL DEFAULT FALSE,
    deleted_at    TIMESTAMPTZ,

    -- Optimistic locking
    version       INTEGER NOT NULL DEFAULT 1,

    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    completed_at  TIMESTAMPTZ
);

CREATE INDEX idx_tasks_user_status   ON tasks(user_id, status) WHERE NOT is_deleted;
CREATE INDEX idx_tasks_user_deadline ON tasks(user_id, deadline) WHERE deadline IS NOT NULL AND NOT is_deleted;
CREATE INDEX idx_tasks_user_active   ON tasks(user_id, created_at DESC) WHERE status IN ('new','in_progress') AND NOT is_deleted;
CREATE INDEX idx_tasks_title_trgm    ON tasks USING gin(title gin_trgm_ops);

-- ── task_tags (M:N) ────────────────────────────────────────────────────────────
CREATE TABLE task_tags (
    task_id UUID NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    tag_id  UUID NOT NULL REFERENCES tags(id)  ON DELETE CASCADE,
    PRIMARY KEY (task_id, tag_id)
);

-- ── subtasks ───────────────────────────────────────────────────────────────────
CREATE TABLE subtasks (
    id           UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    task_id      UUID NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    user_id      UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    title        VARCHAR(512) NOT NULL,
    is_done      BOOLEAN NOT NULL DEFAULT FALSE,
    sort_order   INTEGER NOT NULL DEFAULT 0,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    completed_at TIMESTAMPTZ
);

CREATE INDEX idx_subtasks_task ON subtasks(task_id);

-- ── task_history ───────────────────────────────────────────────────────────────
CREATE TABLE task_history (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    task_id         UUID NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    user_id         UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    action          history_action NOT NULL,
    field_name      VARCHAR(64),
    old_value       JSONB,
    new_value       JSONB,
    comment         TEXT,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_task_history_task ON task_history(task_id, created_at DESC);

-- ── focus_sessions ─────────────────────────────────────────────────────────────
CREATE TABLE focus_sessions (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    task_id         UUID REFERENCES tasks(id) ON DELETE SET NULL,

    mode            session_mode   NOT NULL,
    status          session_status NOT NULL DEFAULT 'active',

    planned_duration_minutes INTEGER NOT NULL,
    actual_duration_minutes  INTEGER NOT NULL DEFAULT 0,

    pomodoro_count       INTEGER NOT NULL DEFAULT 0,
    completed_pomodoros  INTEGER NOT NULL DEFAULT 0,

    interruption_count   INTEGER NOT NULL DEFAULT 0,
    focus_debt_minutes   INTEGER NOT NULL DEFAULT 0,

    notes       TEXT,
    started_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    paused_at   TIMESTAMPTZ,
    ended_at    TIMESTAMPTZ,

    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Только одна активная сессия на пользователя
CREATE UNIQUE INDEX idx_sessions_one_active
    ON focus_sessions(user_id) WHERE status = 'active';

CREATE INDEX idx_sessions_user_date
    ON focus_sessions(user_id, started_at DESC);

-- ── reminders ──────────────────────────────────────────────────────────────────
CREATE TABLE reminders (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    task_id         UUID REFERENCES tasks(id) ON DELETE SET NULL,

    message         TEXT NOT NULL,
    remind_at       TIMESTAMPTZ NOT NULL,
    is_sent         BOOLEAN NOT NULL DEFAULT FALSE,
    sent_at         TIMESTAMPTZ,
    send_attempts   INTEGER NOT NULL DEFAULT 0,

    is_recurring    BOOLEAN NOT NULL DEFAULT FALSE,
    recurrence_rule VARCHAR(256),
    next_remind_at  TIMESTAMPTZ,

    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_reminders_due
    ON reminders(remind_at) WHERE is_sent = FALSE;
CREATE INDEX idx_reminders_user
    ON reminders(user_id, remind_at DESC);

-- ── goals ──────────────────────────────────────────────────────────────────────
CREATE TABLE goals (
    id                      UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id                 UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    type                    goal_type NOT NULL,
    target_focus_minutes    INTEGER NOT NULL DEFAULT 120,
    target_tasks_count      INTEGER NOT NULL DEFAULT 3,
    period_start            DATE NOT NULL,
    period_end              DATE NOT NULL,
    achieved_focus_minutes  INTEGER NOT NULL DEFAULT 0,
    achieved_tasks_count    INTEGER NOT NULL DEFAULT 0,
    is_active               BOOLEAN NOT NULL DEFAULT TRUE,
    created_at              TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at              TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (user_id, type, period_start)
);

CREATE INDEX idx_goals_user_active ON goals(user_id, type, period_end)
    WHERE is_active = TRUE;

-- ── activity_log ───────────────────────────────────────────────────────────────
CREATE TABLE activity_log (
    id          UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id     UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    event_type  VARCHAR(64) NOT NULL,
    event_data  JSONB,
    occurred_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_activity_user ON activity_log(user_id, occurred_at DESC);
CREATE INDEX idx_activity_type  ON activity_log(event_type, occurred_at DESC);

-- ── idempotency_keys ───────────────────────────────────────────────────────────
CREATE TABLE idempotency_keys (
    key         VARCHAR(256) PRIMARY KEY,
    result      JSONB,
    user_id     UUID REFERENCES users(id) ON DELETE CASCADE,
    operation   VARCHAR(128),
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    expires_at  TIMESTAMPTZ NOT NULL
);

CREATE INDEX idx_idempotency_expires ON idempotency_keys(expires_at);

-- ── updated_at triggers ────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION trg_fn_set_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_users_updated_at
    BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION trg_fn_set_updated_at();

CREATE TRIGGER trg_tasks_updated_at
    BEFORE UPDATE ON tasks
    FOR EACH ROW EXECUTE FUNCTION trg_fn_set_updated_at();

CREATE TRIGGER trg_sessions_updated_at
    BEFORE UPDATE ON focus_sessions
    FOR EACH ROW EXECUTE FUNCTION trg_fn_set_updated_at();

CREATE TRIGGER trg_goals_updated_at
    BEFORE UPDATE ON goals
    FOR EACH ROW EXECUTE FUNCTION trg_fn_set_updated_at();

COMMIT;
