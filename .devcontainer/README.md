# FocusForge — Dev Container

## Требования

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) ≥ 4.x
- [VS Code](https://code.visualstudio.com/)
- Расширение [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

## Быстрый старт

```bash
# 1. Клонируем проект
git clone https://github.com/your-org/focusforge.git
cd focusforge

# 2. Копируем env
cp .env.example .env
# Заполняем TELEGRAM_BOT_TOKEN если нужно тестировать webhook

# 3. Открываем в VS Code
code .
```

**В VS Code:** нажмите `F1` → **Dev Containers: Reopen in Container**

Контейнер запустится, автоматически:
- поднимет PostgreSQL, MongoDB, Redis
- применит миграции
- настроит CMake
- установит все расширения VS Code

## Команды

Открыть терминал внутри контейнера (`Ctrl+```) и выполнить:

```bash
make build      # собрать
make run        # запустить
make test       # unit тесты
make help       # все команды
```

## Структура Dev Container

```
.devcontainer/
├── devcontainer.json              # Конфигурация Dev Container
├── Dockerfile                     # Dev образ (userver + toolchain + CLI-tools)
├── docker-compose.devcontainer.yml # Добавляет focusforge-dev сервис
├── post-create.sh                 # Выполняется 1 раз при создании
├── post-start.sh                  # Выполняется при каждом старте
└── .env.devcontainer              # Dev переменные окружения
```

## Порты внутри контейнера

| Порт  | Сервис        |
|-------|---------------|
| 8080  | FocusForge App|
| 5432  | PostgreSQL    |
| 6379  | Redis         |
| 27017 | MongoDB       |

Сервисы доступны по именам: `postgres`, `redis`, `mongo`.

## Отладка

Используйте задачу **"FocusForge (LLDB Debug)"** из панели Run & Debug (`F5`).

Для точки останова: кликните левее номера строки в `.cpp` файле.
