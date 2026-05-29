# FocusForge — Telegram Task & Focus Manager
## Архитектурная документация

---

## 1. Обзор системы

FocusForge — это production-ready Telegram-бот для управления задачами и фокусировкой. Пользователь взаимодействует исключительно через Telegram, backend построен на C++17 + userver.

### Технологический стек

| Компонент       | Технология             | Роль                                            |
|-----------------|------------------------|-------------------------------------------------|
| Backend         | C++17 + userver        | Бизнес-логика, API, обработка webhook           |
| Primary DB      | PostgreSQL 15          | Основное хранилище (users, tasks, sessions...) |
| Document DB     | MongoDB 7              | Preferences, event logs, агрегированные отчёты |
| Cache / Queue   | Redis 7                | Кеш, rate-limit, FSM-состояния, locks           |
| Reverse Proxy   | Nginx 1.25             | TLS termination, routing, rate-limit на входе   |
| Container       | Docker / Compose       | Локальный запуск и CI                           |

---

## 2. Схема компонентов

```
┌─────────────────────────────────────────────────────────────────┐
│                         TELEGRAM API                            │
└────────────────────────────┬────────────────────────────────────┘
                             │ HTTPS Webhook POST /webhook/{token}
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                          NGINX                                  │
│   - TLS termination                                             │
│   - Rate limiting (1000 req/min per IP)                        │
│   - Proxy pass → userver :8080                                  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                    userver Application                          │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                   HTTP Handlers Layer                    │   │
│  │  TelegramWebhookHandler  HealthHandler  MetricsHandler   │   │
│  └───────────────────────┬──────────────────────────────────┘   │
│                          │                                      │
│  ┌───────────────────────▼──────────────────────────────────┐   │
│  │                  Telegram Layer                          │   │
│  │   UpdateDispatcher → CommandRouter → ScenarioManager     │   │
│  │   CallbackRouter → InlineKeyboardBuilder                 │   │
│  └───────────────────────┬──────────────────────────────────┘   │
│                          │                                      │
│  ┌───────────────────────▼──────────────────────────────────┐   │
│  │                  Services Layer                          │   │
│  │  UserService  TaskService  FocusService  AnalyticsService│   │
│  │  ReminderService  NotificationService  ReportService     │   │
│  └──────────┬────────────────────────────┬──────────────────┘   │
│             │                            │                      │
│  ┌──────────▼────────────┐  ┌───────────▼──────────────────┐   │
│  │  Repository Layer     │  │    Cache Layer                │   │
│  │  UserRepository       │  │    UserCache                  │   │
│  │  TaskRepository       │  │    TaskCache                  │   │
│  │  SessionRepository    │  │    SessionStateCache          │   │
│  │  ReminderRepository   │  │    ConversationStateCache     │   │
│  │  AnalyticsRepository  │  │    RateLimitCache             │   │
│  │  MongoRepository      │  │    DistributedLock            │   │
│  └──────────┬────────────┘  └───────────┬──────────────────┘   │
│             │                           │                      │
└─────────────┼───────────────────────────┼──────────────────────┘
              │                           │
    ┌─────────▼──────────┐    ┌──────────▼──────────┐
    │    PostgreSQL       │    │       Redis          │
    │  + MongoDB          │    │                      │
    └────────────────────┘    └─────────────────────┘
```

---

## 3. Доменная модель

### Основные сущности

```
User
├── id (uuid)
├── telegram_id (bigint, unique)
├── username
├── first_name, last_name
├── timezone
├── language_code
├── settings → MongoDB
├── created_at, updated_at
└── is_active

Task
├── id (uuid)
├── user_id → User
├── title, description
├── status: new|in_progress|paused|done|archived
├── priority: low|medium|high|critical
├── deadline (timestamp with tz)
├── estimated_minutes
├── actual_minutes
├── tags[] → TaskTag
├── subtasks[] → Subtask
├── parent_task_id (nullable, self-ref)
├── is_recurring, recurrence_rule
├── is_deleted (soft delete)
└── history[] → TaskHistory

FocusSession
├── id (uuid)
├── user_id → User
├── task_id → Task (nullable)
├── mode: pomodoro|deep_work|custom
├── planned_duration_minutes
├── actual_duration_minutes
├── status: active|paused|completed|cancelled
├── started_at, ended_at
├── breaks[] → SessionBreak
└── notes

Reminder
├── id (uuid)
├── user_id → User
├── task_id → Task (nullable)
├── message
├── remind_at
├── is_sent, sent_at
└── recurrence_rule

Goal (дневные/недельные цели по фокусу)
├── id (uuid)
├── user_id → User
├── type: daily|weekly
├── target_focus_minutes
├── target_tasks_count
├── period_start, period_end
└── is_active
```

---

## 4. Слои архитектуры

### 4.1 Handlers
Принимают HTTP-запросы, валидируют JWT/подпись Telegram, формируют DTO и передают в Telegram Layer. Не содержат бизнес-логики.

### 4.2 Telegram Layer
- **UpdateDispatcher** — разбирает Update от Telegram, маршрутизирует по типу (message, callback_query, etc.)
- **CommandRouter** — маршрутизация по командам (`/start`, `/task`, `/focus`...)
- **ScenarioManager** — конечный автомат (FSM) для многошаговых диалогов. Состояния хранятся в Redis с TTL 30 минут.
- **CallbackRouter** — обработка inline-кнопок
- **InlineKeyboardBuilder** — генерация клавиатур

### 4.3 Services
Бизнес-логика. Транзакции. Идемпотентность. Валидация инвариантов. Не знают о Telegram.

### 4.4 Repositories
Тонкие обёртки над БД. Только CRUD + специфичные запросы. Логика — только запросы, никакой бизнес-логики.

### 4.5 Cache Layer
Redis-клиент с типизированными методами. Сериализация через userver JSON. TTL по типу данных.

---

## 5. Стратегия кеширования

| Данные                  | TTL      | Инвалидация                    |
|-------------------------|----------|--------------------------------|
| Профиль пользователя    | 5 мин    | При update user                |
| Список задач            | 2 мин    | При create/update/delete task  |
| Активная сессия фокуса  | 1 ч      | При end session                |
| Состояние разговора FSM | 30 мин   | При завершении сценария        |
| Rate limit counters     | 1 мин    | Sliding window                 |
| Дневная статистика      | 10 мин   | При create session             |

**Cache-aside** паттерн для профиля и задач.
**Write-through** для активных сессий фокуса.

---

## 6. Идемпотентность и concurrency safety

- Все write-операции принимают `idempotency_key` (UUID из Telegram update_id)
- Distributed lock через Redis SET NX EX для критических секций (старт сессии)
- Optimistic locking через `version` поле в PostgreSQL для задач
- UPSERT вместо INSERT для операций регистрации пользователя

---

## 7. Telegram FSM (диалоговые сценарии)

```
Сценарий создания задачи:
IDLE → TASK_TITLE → TASK_PRIORITY → TASK_DEADLINE → TASK_TAGS → CONFIRM → DONE

Сценарий фокус-сессии:
IDLE → SELECT_MODE → SELECT_TASK → CONFIRM_DURATION → SESSION_ACTIVE
      └→ SESSION_PAUSED → SESSION_ACTIVE (resume)
      └→ SESSION_COMPLETED → SHOW_STATS

Сценарий напоминания:
IDLE → REMINDER_MESSAGE → REMINDER_TIME → REMINDER_TASK_LINK → CONFIRM → DONE
```

Состояние FSM хранится в Redis как JSON:
```json
{
  "user_id": 123456789,
  "state": "TASK_PRIORITY",
  "data": {"title": "Написать отчёт"},
  "expires_at": "2024-01-15T14:30:00Z"
}
```

---

## 8. Метрики и наблюдаемость

### Prometheus-совместимые метрики:
- `focusforge_requests_total{handler,status}`
- `focusforge_request_duration_seconds{handler,quantile}`
- `focusforge_tasks_created_total{priority}`
- `focusforge_focus_sessions_total{mode,status}`
- `focusforge_active_users_gauge`
- `focusforge_db_query_duration_seconds{query_type}`
- `focusforge_cache_hit_rate{cache_type}`
- `focusforge_errors_total{layer,error_type}`

### Structured logging (JSON):
```json
{
  "timestamp": "2024-01-15T12:00:00.123Z",
  "level": "INFO",
  "request_id": "uuid",
  "telegram_user_id": 123456789,
  "action": "task_created",
  "task_id": "uuid",
  "duration_ms": 45
}
```

---

## 9. PostgreSQL — индексы

```sql
-- Частые запросы по пользователю и статусу
CREATE INDEX idx_tasks_user_status ON tasks(user_id, status) WHERE NOT is_deleted;
CREATE INDEX idx_tasks_user_deadline ON tasks(user_id, deadline) WHERE deadline IS NOT NULL AND NOT is_deleted;
CREATE INDEX idx_tasks_user_priority ON tasks(user_id, priority, created_at DESC) WHERE NOT is_deleted;

-- Напоминания к отправке
CREATE INDEX idx_reminders_pending ON reminders(remind_at) WHERE NOT is_sent;

-- Сессии фокуса
CREATE INDEX idx_sessions_user_date ON focus_sessions(user_id, started_at DESC);
CREATE INDEX idx_sessions_active ON focus_sessions(user_id) WHERE status = 'active';

-- Аналитика
CREATE INDEX idx_sessions_user_period ON focus_sessions(user_id, started_at)
  WHERE status = 'completed';
```

---

## 10. Порядок запуска

```bash
# 1. Клонирование и настройка
git clone https://github.com/your-org/focusforge.git
cd focusforge
cp .env.example .env
# Отредактируйте .env: TELEGRAM_BOT_TOKEN, POSTGRES_PASSWORD, etc.

# 2. Запуск всех сервисов
docker compose up -d --build

# 3. Применение миграций (автоматически через init-контейнер)
# Или вручную:
docker compose exec postgres psql -U focusforge -d focusforge -f /migrations/001_initial.sql

# 4. Регистрация Webhook
curl -X POST "https://api.telegram.org/bot${BOT_TOKEN}/setWebhook" \
  -d "url=https://your-domain.com/webhook/${BOT_TOKEN}"

# 5. Проверка здоровья
curl http://localhost:8080/health

# 6. Запуск тестов
docker compose -f docker-compose.test.yml up --abort-on-container-exit
```
