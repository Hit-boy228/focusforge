-- =============================================================================
-- Migration 004: Task History Triggers (Audit)
-- =============================================================================

BEGIN;

-- Автоматическое логирование изменений задач через триггер
-- Дополняет явные записи из application layer

CREATE OR REPLACE FUNCTION trg_fn_task_history_auto()
RETURNS TRIGGER AS $$
DECLARE
    v_action history_action;
BEGIN
    -- Определяем действие
    IF (TG_OP = 'INSERT') THEN
        v_action := 'created';
    ELSIF (TG_OP = 'UPDATE') THEN
        IF NEW.is_deleted AND NOT OLD.is_deleted THEN
            v_action := 'deleted';
        ELSIF NEW.status != OLD.status THEN
            v_action := 'status_changed';
        ELSIF NEW.priority != OLD.priority THEN
            v_action := 'priority_changed';
        ELSIF NEW.deadline IS DISTINCT FROM OLD.deadline THEN
            v_action := 'deadline_changed';
        ELSE
            v_action := 'updated';
        END IF;
    END IF;

    -- Записываем только значимые изменения (не технические поля типа updated_at)
    IF v_action IS NOT NULL THEN
        INSERT INTO task_history (
            id, task_id, user_id, action,
            field_name, old_value, new_value,
            created_at
        ) VALUES (
            uuid_generate_v4(),
            COALESCE(NEW.id, OLD.id),
            COALESCE(NEW.user_id, OLD.user_id),
            v_action,
            CASE v_action
                WHEN 'status_changed'   THEN 'status'
                WHEN 'priority_changed' THEN 'priority'
                WHEN 'deadline_changed' THEN 'deadline'
                ELSE NULL
            END,
            CASE v_action
                WHEN 'status_changed'   THEN to_jsonb(OLD.status::text)
                WHEN 'priority_changed' THEN to_jsonb(OLD.priority::text)
                WHEN 'deadline_changed' THEN to_jsonb(OLD.deadline)
                ELSE NULL
            END,
            CASE v_action
                WHEN 'status_changed'   THEN to_jsonb(NEW.status::text)
                WHEN 'priority_changed' THEN to_jsonb(NEW.priority::text)
                WHEN 'deadline_changed' THEN to_jsonb(NEW.deadline)
                ELSE NULL
            END,
            NOW()
        );
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Устанавливаем триггер только для создания и update status/priority/deadline
-- Intentional: мы логируем через application layer тоже,
-- но триггер служит резервным механизмом для прямых DB изменений
CREATE TRIGGER trg_task_history_auto
    AFTER INSERT OR UPDATE OF status, priority, deadline, is_deleted
    ON tasks
    FOR EACH ROW
    EXECUTE FUNCTION trg_fn_task_history_auto();

COMMIT;
