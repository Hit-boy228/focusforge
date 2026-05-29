# Deployment Guide

## Local Development

```bash
cp .env.example .env
# Edit .env with your tokens
./scripts/run_local.sh
```

## Production Deployment

### Requirements
- Docker & Docker Compose
- Domain with TLS (Nginx handles SSL termination)
- Telegram Bot Token

### Steps

1. **Set environment variables:**
```bash
export TELEGRAM_BOT_TOKEN=...
export TELEGRAM_WEBHOOK_SECRET=...
export POSTGRES_PASSWORD=...
export MONGO_PASSWORD=...
export REDIS_PASSWORD=...
```

2. **Deploy:**
```bash
docker compose -f docker-compose.yml up -d
./scripts/apply_migrations.sh
```

3. **Register webhook:**
```bash
curl -X POST "https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/setWebhook" \
     -d "url=https://your-domain.com/webhook/${TELEGRAM_BOT_TOKEN}" \
     -d "secret_token=${TELEGRAM_WEBHOOK_SECRET}"
```

4. **Verify:**
```bash
curl https://your-domain.com/health/live
```

## Scaling

- App is stateless — scale horizontally
- Session state in Redis → shared across instances
- Use PostgreSQL connection pooling (PgBouncer) for > 5 instances

## Zero-Downtime Deploys

1. Deploy new version (new container)
2. Health check passes
3. Swap traffic (Nginx upstream update or load balancer)
4. Stop old container

Active sessions survive — recovered from PostgreSQL on startup.
