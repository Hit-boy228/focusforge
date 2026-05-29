"""tests/functional/test_error_paths.py — проверка граничных случаев"""
import pytest
import httpx

BASE_URL = "http://localhost:8080"
BOT_TOKEN = "test_token"
SECRET   = "test_secret"
TG_USER  = 555555


def post_update(client, uid, text):
    return client.post(
        f"/webhook/{BOT_TOKEN}",
        json={"update_id": uid, "message": {
            "message_id": uid,
            "from": {"id": TG_USER, "is_bot": False, "first_name": "Dave",
                     "language_code": "en"},
            "chat": {"id": TG_USER, "type": "private"},
            "text": text, "date": 1700000000
        }},
        headers={"X-Telegram-Bot-Api-Secret-Token": SECRET}
    )


@pytest.fixture
def client():
    return httpx.Client(base_url=BASE_URL, timeout=10)


def test_unknown_command_returns_200(client):
    r = post_update(client, 5001, "/unknown_command_xyz")
    assert r.status_code == 200  # бот отвечает, но не ломается

def test_empty_text_returns_200(client):
    r = post_update(client, 5002, "")
    assert r.status_code == 200

def test_very_long_text_returns_200(client):
    r = post_update(client, 5003, "x" * 4096)
    assert r.status_code == 200

def test_cancel_command_clears_state(client):
    post_update(client, 5010, "/task")  # начинаем сценарий
    r = post_update(client, 5011, "/cancel")  # отменяем
    assert r.status_code == 200

def test_done_with_nonexistent_id(client):
    r = post_update(client, 5020, "/done 00000000-0000-0000-0000-000000000000")
    assert r.status_code == 200  # 404 внутри бота, не HTTP
