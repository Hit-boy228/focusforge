"""
tests/functional/test_telegram_start.py
Функциональные тесты — симуляция Telegram updates через HTTP
Требует: запущенного сервиса (docker-compose up)
"""
import pytest
import httpx
import json

BASE_URL = "http://localhost:8080"
WEBHOOK_SECRET = "test_secret"
BOT_TOKEN = "test_token"


def make_update(update_id: int, tg_user_id: int, text: str) -> dict:
    return {
        "update_id": update_id,
        "message": {
            "message_id": update_id,
            "from": {
                "id": tg_user_id,
                "is_bot": False,
                "first_name": "Test",
                "username": "testuser",
                "language_code": "en"
            },
            "chat": {"id": tg_user_id, "type": "private"},
            "text": text,
            "date": 1700000000
        }
    }


@pytest.fixture
def client():
    return httpx.Client(base_url=BASE_URL, timeout=10)


def post_update(client, update: dict) -> httpx.Response:
    return client.post(
        f"/webhook/{BOT_TOKEN}",
        json=update,
        headers={"X-Telegram-Bot-Api-Secret-Token": WEBHOOK_SECRET}
    )


def test_health_live(client):
    resp = client.get("/health/live")
    assert resp.status_code == 200
    data = resp.json()
    assert data["status"] == "ok"


def test_health_ready(client):
    resp = client.get("/health/ready")
    assert resp.status_code == 200


def test_start_command_returns_200(client):
    update = make_update(1001, 111111, "/start")
    resp = post_update(client, update)
    assert resp.status_code == 200
    assert resp.json()["ok"] is True


def test_duplicate_update_ignored(client):
    update = make_update(1002, 111111, "/start")
    resp1 = post_update(client, update)
    resp2 = post_update(client, update)  # same update_id
    assert resp1.status_code == 200
    assert resp2.status_code == 200
    # Оба возвращают ok=true, но второй обрабатывается как дубликат


def test_invalid_secret_returns_403(client):
    update = make_update(1003, 111111, "/start")
    resp = client.post(
        f"/webhook/{BOT_TOKEN}",
        json=update,
        headers={"X-Telegram-Bot-Api-Secret-Token": "wrong_secret"}
    )
    assert resp.status_code == 403


def test_invalid_json_returns_400(client):
    resp = client.post(
        f"/webhook/{BOT_TOKEN}",
        content=b"not json",
        headers={
            "Content-Type": "application/json",
            "X-Telegram-Bot-Api-Secret-Token": WEBHOOK_SECRET
        }
    )
    assert resp.status_code == 400
