# FocusForge 🎯

> **⚠️ ПРОЕКТ В АКТИВНОЙ РАЗРАБОТКЕ — НЕ ГОТОВ К PRODUCTION**  
> Многие функции реализованы частично. API и структура могут меняться без предупреждения.

C++ Telegram-бот для управления задачами и продуктивностью.  
Стек: **C++17 · userver · PostgreSQL 15 · MongoDB 7 · Redis 7 · Docker**

---

## Статус

| Компонент | Статус |
|---|---|
| Telegram webhook + маршрутизация | ✅ Работает |
| Команды `/start`, `/task`, `/tasks`, `/focus` | ✅ Работают |
| Команды `/stats`, `/week`, `/streak`, `/goals`, `/settings`, `/help` | ✅ Работают |
| PostgreSQL репозитории | ✅ Работают |
| Redis Sentinel | ✅ Работает |
| `/health/live` эндпоинт | ✅ Работает |
| MongoDB репозитории | 🔧 В разработке |
| Callback-кнопки (inline keyboards) | 🔧 В разработке |
| Тесты (unit / integration) | 🔧 В разработке |
| Nginx / production deploy | 🔧 В разработке |

---

## Архитектура

```
Telegram API ──→ localhost.run tunnel ──→ userver App ──→ PostgreSQL (source of truth)
                                                       ──→ MongoDB    (preferences, event logs)
                                                       ──→ Redis      (cache, locks, FSM state)
```

Туннель поднимается автоматически при `docker compose up` — никаких дополнительных действий не нужно.

---

## Быстрый старт

### Требования

- Docker ≥ 24.0 и Docker Compose ≥ 2.20
- Telegram Bot Token — получить у [@BotFather](https://t.me/botfather)

### 1. Клонирование

```bash
git clone https://github.com/Hit-boy228/focusforge.git
cd focusforge
```

### 2. Настройка

```bash
cp .env.example .env
cp configs/config_vars.example.yaml configs/config_vars.yaml
cp configs/secrets.example.json configs/secdist.json
```

Откройте `.env` и задайте:

| Переменная | Что вписать |
|---|---|
| `TELEGRAM_BOT_TOKEN` | Токен от [@BotFather](https://t.me/botfather) |
| `TELEGRAM_WEBHOOK_SECRET` | Любая случайная строка ≥ 32 символа |
| `POSTGRES_PASSWORD` | Придумайте пароль |
| `MONGO_PASSWORD` | Придумайте пароль |

Откройте `configs/config_vars.yaml` и пропишите **те же пароли** в строках `pg_dsn` и `mongo_dsn`.

> `configs/secdist.json` для локального запуска редактировать не нужно.

### 3. Запуск

```bash
docker compose up -d --build
```

После старта контейнер `tunnel` автоматически:
1. Открывает HTTPS-туннель через [localhost.run](https://localhost.run)
2. Регистрирует webhook в Telegram

Проверить можно через логи:

```bash
docker compose logs -f tunnel
```

Ожидаемый вывод:
```
[tunnel] Connecting localhost.run → http://focusforge:8080
[tunnel] URL: https://xxxxxxxxxxxxxxxx.lhr.life
[tunnel] setWebhook → https://xxxxxxxxxxxxxxxx.lhr.life/webhook/<token>
[tunnel] Telegram: {"ok":true,"result":true,"description":"Webhook was set"}
```

> **Внимание:** URL туннеля меняется при каждом рестарте контейнера — webhook перерегистрируется автоматически.

---

## Команды бота

| Команда | Описание |
|---|---|
| `/start` | Регистрация / приветствие |
| `/task`, `/newtask` | Создать задачу |
| `/tasks` | Список задач |
| `/today`, `/plan` | Умный план на сегодня |
| `/focus` | Запустить фокус-сессию (Pomodoro / Deep Work) |
| `/stop`, `/pause` | Управление сессией |
| `/stats` | Статистика за сегодня |
| `/week` | Итоги недели |
| `/streak` | Текущий стрик |
| `/goals` | Прогресс дневных целей |
| `/remind` | Создать напоминание |
| `/reminders` | Список напоминаний |
| `/review` | Еженедельный обзор |
| `/settings` | Настройки профиля |
| `/help` | Справка |
| `/cancel` | Отменить текущее действие |

---

## Структура проекта

```
src/
├── app/            — регистрация компонентов (bootstrap)
├── core/           — базовые типы, Result<T>, ошибки, утилиты времени
├── domain/         — доменные сущности (чистые, без HTTP/DB)
├── dto/            — DTO для границ между слоями
├── validators/     — валидация входных данных
├── repositories/
│   ├── postgres/   — PostgreSQL (задачи, пользователи, сессии, ...)
│   ├── redis/      — кеш, блокировки, состояние FSM
│   └── mongo/      — настройки, event log, снимки отчётов
├── services/       — бизнес-логика
├── handlers/       — HTTP handlers (userver)
├── telegram/       — Telegram UX: router, scenes, keyboards, шаблоны
├── observability/  — метрики, tracing
└── utils/          — вспомогательные утилиты
configs/
├── static_config.yaml           — userver конфигурация компонентов
├── config_vars.example.yaml     — ШАБЛОН переменных → скопировать в config_vars.yaml
├── secrets.example.json         — ШАБЛОН secdist    → скопировать в secdist.json
└── secdist.json                 — [gitignored] настройки Redis Sentinel
deploy/
├── tunnel/                      — контейнер SSH-туннеля (localhost.run)
├── migrator/                    — скрипт применения SQL-миграций
└── postgres/                    — конфиг PostgreSQL
migrations/                      — SQL миграции
```

---

## Переменные окружения

| Переменная | Обязательная | Описание |
|---|:---:|---|
| `TELEGRAM_BOT_TOKEN` | ✅ | Токен бота от BotFather |
| `TELEGRAM_WEBHOOK_SECRET` | ✅ | Секрет для верификации webhook (≥ 32 символа) |
| `POSTGRES_PASSWORD` | ✅ | Пароль PostgreSQL — должен совпадать с `pg_dsn` в `config_vars.yaml` |
| `MONGO_PASSWORD` | ✅ | Пароль MongoDB — должен совпадать с `mongo_dsn` в `config_vars.yaml` |
| `REDIS_PASSWORD` | ❌ | Пароль Redis (в dev не нужен) |
| `LOG_LEVEL` | ❌ | `debug` / `info` / `warning` (по умолчанию `debug`) |

---

## Разработка

```bash
# Пересборка приложения
docker compose build focusforge

# Логи всех сервисов
docker compose logs -f

# Логи конкретного сервиса
docker compose logs -f focusforge
docker compose logs -f tunnel

# Подключиться к PostgreSQL
docker compose exec postgres psql -U focusforge -d focusforge

# Статус webhook
curl -s "https://api.telegram.org/bot<TOKEN>/getWebhookInfo" | python3 -m json.tool
```

---

## Лицензия

MIT © 2024–2025 [Hit-boy228](https://github.com/Hit-boy228)
