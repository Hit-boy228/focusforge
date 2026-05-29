"""
tests/e2e/test_restart_recovery.py
Проверяет восстановление сессии после рестарта сервиса.
Требует: docker-compose, возможности перезапуска контейнера.
"""
import pytest
import httpx
import subprocess
import time

BASE_URL = "http://localhost:8080"
BOT_TOKEN = "test_token"
SECRET = "test_secret"
TG_USER = 777002


def webhook(client, uid, text):
    return client.post(
        f"/webhook/{BOT_TOKEN}",
        json={"update_id": uid, "message": {
            "message_id": uid,
            "from": {"id": TG_USER, "is_bot": False,
                     "first_name": "RecoveryUser", "language_code": "en"},
            "chat": {"id": TG_USER, "type": "private"},
            "text": text, "date": int(time.time())
        }},
        headers={"X-Telegram-Bot-Api-Secret-Token": SECRET}
    )


@pytest.fixture
def client():
    return httpx.Client(base_url=BASE_URL, timeout=15)


@pytest.mark.skip(reason="Requires docker-compose control — run manually")
def test_session_survives_restart(client):
    # 1. Регистрируем пользователя
    webhook(client, 8001, "/start")

    # 2. Запускаем сессию
    webhook(client, 8002, "/focus 25m")

    # 3. Рестартуем контейнер приложения
    subprocess.run(["docker", "compose", "restart", "focusforge"],
                   check=True)
    time.sleep(5)

    # 4. После рестарта сессия должна восстановиться
    r = webhook(client, 8003, "/focus")
    assert r.status_code == 200
    # Бот должен показать текущую активную сессию (не создать новую)
