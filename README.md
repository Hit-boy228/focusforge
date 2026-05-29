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
| MongoDB репозитории | 🔧 В разработке |
| Callback-кнопки (inline keyboards) | 🔧 В разработке |
| Тесты (unit / integration) | 🔧 В разработке |
| Nginx / production deploy | 🔧 В разработке |
| `/health/live` эндпоинт | ❌ Не реализован |

---

## Архитектура

```
Telegram API ──→ cloudflared tunnel ──→ userver App ──→ PostgreSQL (source of truth)
                                                      ──→ MongoDB    (preferences, event logs)
                                                      ──→ Redis      (cache, locks, FSM state)
```

Подробнее: [docs/architecture.md](docs/architecture.md) · [ARCHITECTURE.md](ARCHITECTURE.md)

---

## Быстрый старт (локально)

### Требования

- Docker ≥ 24.0 и Docker Compose ≥ 2.20
- Telegram Bot Token — получить у [@BotFather](https://t.me/botfather)

### 1. Клонирование

```bash
git clone https://github.com/YOUR_USERNAME/focusforge.git
cd focusforge
```

### 2. Настройка секретов

```bash
# Скопируйте шаблоны и заполните реальными значениями
cp .env.example .env
cp configs/config_vars.example.yaml configs/config_vars.yaml
cp configs/secrets.example.json configs/secdist.json
```

Откройте `.env` и `configs/config_vars.yaml` — заполните:
- `TELEGRAM_BOT_TOKEN` — токен от BotFather
- `TELEGRAM_WEBHOOK_SECRET` — произвольная случайная строка (≥ 32 символа)
- Пароли для PostgreSQL, MongoDB, Redis

### 3. Запуск

```bash
docker compose up -d --build
```

### 4. Webhook через cloudflared (локальный dev)

```bash
# Установите cloudflared: https://developers.cloudflare.com/cloudflare-one/connections/connect-apps/install-and-setup/
cloudflared tunnel --url http://localhost:8080 --no-autoupdate > /tmp/cf.log 2>&1 &
sleep 8

TUNNEL_URL=$(grep -Eo "https://[a-zA-Z0-9-]+\.trycloudflare\.com" /tmp/cf.log | head -1)

curl -X POST "https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/setWebhook" \
  -H "Content-Type: application/json" \
  -d "{\"url\":\"${TUNNEL_URL}/webhook/${TELEGRAM_BOT_TOKEN}\",\"secret_token\":\"${TELEGRAM_WEBHOOK_SECRET}\"}"
```

> **Внимание:** URL туннеля меняется при каждом перезапуске cloudflared — нужно перерегистрировать webhook.

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
├── static_config.yaml         — userver конфигурация компонентов
├── config_vars.example.yaml   — ШАБЛОН переменных (скопируйте → config_vars.yaml)
├── secrets.example.json       — ШАБЛОН secdist (скопируйте → secdist.json)
└── secdist.json               — [gitignored] реальные настройки Redis
migrations/                    — SQL миграции PostgreSQL
docs/                          — ADR, архитектура, API
```

---

## Переменные окружения

| Переменная | Обязательная | Описание |
|---|:---:|---|
| `TELEGRAM_BOT_TOKEN` | ✅ | Токен бота от BotFather |
| `TELEGRAM_WEBHOOK_SECRET` | ✅ | Секрет для верификации webhook |
| `POSTGRES_PASSWORD` | ✅ | Пароль PostgreSQL |
| `MONGO_PASSWORD` | ✅ | Пароль MongoDB |
| `REDIS_PASSWORD` | ❌ | Пароль Redis (по умолчанию без пароля) |
| `LOG_LEVEL` | ❌ | `debug` / `info` / `warning` |

Полный список: [.env.example](.env.example)

---

## Разработка

```bash
# Пересборка приложения
docker compose build focusforge

# Логи
docker compose logs -f focusforge

# Подключиться к PostgreSQL
docker compose exec postgres psql -U focusforge -d focusforge
```

---

## Лицензия

MIT © 2024–2025 Alex
