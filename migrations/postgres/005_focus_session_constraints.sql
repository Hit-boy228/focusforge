-- =============================================================================
-- Migration 005: Focus Session Constraints
-- =============================================================================

BEGIN;

-- Гарантируем: у пользователя не может быть двух активных сессий
-- (уже есть partial unique index, добавляем CHECK для дополнительной защиты)

-- Функция-проверка для использования в constraint trigger
CREATE OR REPLACE FUNCTION check_no_concurrent_active_sessions()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.status = 'active' THEN
        IF EXISTS (
            SELECT 1 FROM focus_sessions
            WHERE user_id = NEW.user_id
              AND status = 'active'
              AND id != NEW.id
        ) THEN
            RAISE EXCEPTION 'User % already has an active focus session', NEW.user_id
                USING ERRCODE = 'unique_violation';
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE CONSTRAINT TRIGGER trg_check_active_session
    AFTER INSERT OR UPDATE OF status ON focus_sessions
    DEFERRABLE INITIALLY DEFERRED
    FOR EACH ROW
    EXECUTE FUNCTION check_no_concurrent_active_sessions();

-- Продолжительность сессии должна быть разумной
ALTER TABLE focus_sessions
    ADD CONSTRAINT chk_session_duration
        CHECK (planned_duration_minutes BETWEEN 1 AND 480),  -- макс 8 часов
    ADD CONSTRAINT chk_session_actual
        CHECK (actual_duration_minutes >= 0),
    ADD CONSTRAINT chk_pomodoro_count
        CHECK (pomodoro_count >= 0 AND completed_pomodoros <= pomodoro_count);

-- ended_at должен быть после started_at
ALTER TABLE focus_sessions
    ADD CONSTRAINT chk_session_times
        CHECK (ended_at IS NULL OR ended_at >= started_at);

COMMIT;
