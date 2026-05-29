-- tests/fixtures/postgres/seed.sql
-- Тестовые данные для integration и e2e тестов

-- Test users
INSERT INTO users (id, telegram_id, username, first_name, language_code, timezone)
VALUES
    ('00000000-0000-0000-0000-000000000001', 100001, 'alice_test', 'Alice', 'en', 'UTC'),
    ('00000000-0000-0000-0000-000000000002', 100002, 'bob_test',   'Bob',   'ru', 'Europe/Moscow')
ON CONFLICT (telegram_id) DO NOTHING;

-- Test tasks
INSERT INTO tasks (id, user_id, title, status, priority, created_at, updated_at)
VALUES
    ('10000000-0000-0000-0000-000000000001',
     '00000000-0000-0000-0000-000000000001',
     'Write unit tests', 'new', 'high', NOW(), NOW()),
    ('10000000-0000-0000-0000-000000000002',
     '00000000-0000-0000-0000-000000000001',
     'Review PR', 'in_progress', 'medium', NOW() - INTERVAL '2 days', NOW()),
    ('10000000-0000-0000-0000-000000000003',
     '00000000-0000-0000-0000-000000000001',
     'Overdue task', 'new', 'critical',
     NOW() - INTERVAL '7 days', NOW() - INTERVAL '7 days')
ON CONFLICT DO NOTHING;

-- Overdue deadline
UPDATE tasks
SET deadline = NOW() - INTERVAL '1 day'
WHERE id = '10000000-0000-0000-0000-000000000003';

-- Completed focus session for Alice
INSERT INTO focus_sessions (id, user_id, mode, status,
    planned_duration_minutes, actual_duration_minutes,
    pomodoro_count, completed_pomodoros,
    started_at, ended_at, created_at, updated_at)
VALUES
    ('20000000-0000-0000-0000-000000000001',
     '00000000-0000-0000-0000-000000000001',
     'pomodoro', 'completed',
     25, 25, 4, 1,
     NOW() - INTERVAL '3 hours',
     NOW() - INTERVAL '2 hours 35 minutes',
     NOW() - INTERVAL '3 hours',
     NOW() - INTERVAL '2 hours 35 minutes')
ON CONFLICT DO NOTHING;

-- Daily goal for Alice
INSERT INTO goals (id, user_id, type, target_focus_minutes, target_tasks_count,
    period_start, period_end, achieved_focus_minutes, achieved_tasks_count)
VALUES
    ('30000000-0000-0000-0000-000000000001',
     '00000000-0000-0000-0000-000000000001',
     'daily', 120, 3, CURRENT_DATE, CURRENT_DATE, 25, 0)
ON CONFLICT DO NOTHING;

-- User streak
INSERT INTO user_streaks (user_id, current_streak, longest_streak,
    last_active_date, grace_days_used, grace_days_total)
VALUES
    ('00000000-0000-0000-0000-000000000001', 7, 14, CURRENT_DATE, 0, 1)
ON CONFLICT (user_id) DO NOTHING;
