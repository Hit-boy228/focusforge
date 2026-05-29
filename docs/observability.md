# Observability

## Structured Logs

All logs include: `request_id`, `user_id`, `telegram_id`, `operation`, `result`.

Key log events:
- `event=task_created` — task_id, user_id, title_len
- `event=session_started` — session_id, mode, user_id
- `event=session_completed` — session_id, duration_min
- `event=rate_limit_hit` — tg_id, operation
- `event=idempotency_duplicate` — update_id

## Metrics (`/metrics`)

| Metric | Type | Description |
|--------|------|-------------|
| `focusforge.tasks.created` | counter | Total tasks created |
| `focusforge.tasks.completed` | counter | Total tasks completed |
| `focusforge.sessions.started` | counter | Focus sessions started |
| `focusforge.sessions.completed` | counter | Sessions completed |
| `focusforge.reminders.sent` | counter | Reminders delivered |
| `focusforge.webhook.received` | counter | Webhook calls |
| `focusforge.webhook.duplicates` | counter | Deduplicated updates |
| `focusforge.rate_limit.hits` | counter | Rate limit rejections |
| `focusforge.focus.total_minutes` | gauge | Total focus minutes |

## Health Checks

- `GET /health/live` — process alive
- `GET /health/ready` — DB connections OK

## Correlation IDs

Every request gets a `X-Request-Id` header.  
Telegram updates carry `update_id` as correlation key.  
All log lines include `request_id` for tracing.
