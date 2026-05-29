# Data Retention Policy

## PostgreSQL

| Table | Retention | Policy |
|-------|-----------|--------|
| `users` | Forever | Soft delete via `is_active=false` |
| `tasks` | 30 days after soft delete | Cleaned by `cleanup_old_soft_deleted_tasks()` |
| `focus_sessions` | Forever | Archived for analytics |
| `task_history` | 1 year | Periodic cleanup job |
| `activity_log` | 90 days | TTL cleanup function |
| `telegram_processed_updates` | 7 days | Auto-cleanup in `cleanup_expired_idempotency_keys()` |
| `idempotency_keys` | 24 hours | `expires_at` + periodic cleanup |
| `reminders` | Forever (sent ones) | Manual archival |

## MongoDB

| Collection | Retention | Policy |
|-----------|-----------|--------|
| `event_logs` | 90 days | TTL index on `created_at` |
| `report_snapshots` | 30 days | TTL index on `created_at` |
| `user_preferences` | Forever | Deleted with user |

## Redis

| Key Pattern | TTL |
|-------------|-----|
| `user:profile:*` | 5 minutes |
| `session:active:*` | 2 hours |
| `conv:*` | 30 minutes |
| `rl:*` | 60 seconds (rate limit window) |
| `lock:*` | 30 seconds |

## GDPR / User Data Deletion

When a user requests deletion:
1. Set `users.is_active = false`
2. Soft-delete all tasks
3. Cancel active sessions and reminders
4. Delete `user_preferences` from MongoDB
5. Delete from `user_streaks`
6. Schedule full hard-delete after 30 days grace period
