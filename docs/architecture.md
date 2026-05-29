# Architecture

## Overview

FocusForge is a layered C++17 service built on **userver** framework.

```
Telegram API → Nginx (TLS, rate-limit) → userver App
                                              ├─ PostgreSQL  (source of truth)
                                              ├─ MongoDB      (preferences, event logs)
                                              └─ Redis        (cache, locks, FSM)
```

## Layers

| Layer | Package | Responsibility |
|-------|---------|----------------|
| **Handlers** | `src/handlers/` | HTTP boundary — parse request, call service, format response |
| **Telegram** | `src/telegram/` | Telegram UX — router, scenes, keyboards, reply builder |
| **Services** | `src/services/` | Business logic, orchestration, no DB/HTTP details |
| **Repositories** | `src/repositories/` | Data access — one class per storage per entity |
| **Domain** | `src/domain/` | Pure entities, value objects, enums — no dependencies |
| **DTO** | `src/dto/` | Data transfer objects between layers |
| **Core** | `src/core/` | Cross-cutting: errors, Result<T>, time, ids, text |

## Key Design Decisions

- **Handlers never touch SQL** — they call services only.
- **Services never build Telegram messages** — they return domain objects.
- **Repositories never contain business rules** — only data access.
- **Domain stays pure** — no userver, no HTTP, no DB types.
- **Redis is not a database** — it accelerates, not stores truth.
- **MongoDB is not a dump** — only flexible/event/preference data goes there.

## Data Flow (Telegram Update)

```
TelegramWebhookHandler
  → verify secret
  → idempotency check (postgres)
  → rate-limit check (redis)
  → touch user (postgres + redis cache)
  → Router.Route(update)
      → scene.HandleXxx(msg)
          → service.DoSomething(req)
              → repository.Insert/Update(...)
              → cache.Del(...)
              → activity_repo.LogEvent(...)
          → NotificationService.SendMessage(chat_id, text)
```

## Component Registration Order

1. Logging, Tracer, StatisticsStorage
2. Database connections (Postgres, Mongo, Redis)
3. Repositories (depend on DB connections)
4. Services (depend on repositories)
5. Handlers (depend on services)
