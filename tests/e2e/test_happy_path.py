"""
tests/e2e/test_happy_path.py
Полный сценарий: регистрация → задача → сессия → отчёт
"""
import pytest
import httpx
import time

BASE_URL = "http://localhost:8080"
BOT_TOKEN = "test_token"
SECRET = "test_secret"
TG_USER = 777001


def webhook(client, uid, text, user_id=None):
    uid_ = user_id or TG_USER
    resp = client.post(
        f"/webhook/{BOT_TOKEN}",
        json={"update_id": uid, "message": {
            "message_id": uid,
            "from": {"id": uid_, "is_bot": False,
                     "first_name": "E2EUser", "language_code": "en"},
            "chat": {"id": uid_, "type": "private"},
            "text": text, "date": int(time.time())
        }},
        headers={"X-Telegram-Bot-Api-Secret-Token": SECRET}
    )
    assert resp.status_code == 200
    return resp


@pytest.fixture(scope="module")
def client():
    return httpx.Client(base_url=BASE_URL, timeout=15)


def test_full_happy_path(client):
    # 1. Регистрация
    webhook(client, 7001, "/start")

    # 2. Создание задачи
    webhook(client, 7002, "/task")
    webhook(client, 7003, "Write project proposal")  # title
    # (callback для приоритета — в e2e проверяем только что 200)

    # 3. Просмотр задач
    webhook(client, 7004, "/tasks")
    webhook(client, 7005, "/today")

    # 4. Старт фокус-сессии
    webhook(client, 7006, "/focus 25m")

    # 5. Статистика
    webhook(client, 7007, "/stats")

    # 6. Недельный отчёт
    webhook(client, 7008, "/week")

    # Все шаги прошли без 5xx
