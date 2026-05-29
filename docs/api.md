# API Reference

## HTTP Endpoints

### Health

| Method | Path | Description |
|--------|------|-------------|
| GET | `/health/live` | Liveness probe |
| GET | `/health/ready` | Readiness probe (checks DB) |
| GET | `/metrics` | Prometheus-compatible metrics |

### Telegram Webhook

| Method | Path | Description |
|--------|------|-------------|
| POST | `/webhook/{bot_token}` | Telegram update receiver |

Headers required:
- `X-Telegram-Bot-Api-Secret-Token: <webhook_secret>`

### Internal REST (admin/debug)

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/v1/tasks` | List tasks (admin) |
| POST | `/api/v1/admin/cache/flush` | Flush Redis cache |
| POST | `/api/v1/admin/reminders/resend` | Resend reminder |

## Telegram Commands

See [telegram-flows.md](telegram-flows.md) for full dialog flows.

## Error Format

```json
{
  "ok": false,
  "error": "task not found: 00000000-...",
  "code": "not_found",
  "request_id": "abc123"
}
```
