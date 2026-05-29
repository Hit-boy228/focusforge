"""tests/functional/test_focus_flow.py"""
import pytest
import httpx

BASE_URL = "http://localhost:8080"
BOT_TOKEN = "test_token"
SECRET   = "test_secret"
TG_USER  = 333333


def post_update(client, uid, text):
    return client.post(
        f"/webhook/{BOT_TOKEN}",
        json={"update_id": uid, "message": {
            "message_id": uid,
            "from": {"id": TG_USER, "is_bot": False, "first_name": "Bob",
                     "language_code": "en"},
            "chat": {"id": TG_USER, "type": "private"},
            "text": text, "date": 1700000000
        }},
        headers={"X-Telegram-Bot-Api-Secret-Token": SECRET}
    )


@pytest.fixture
def client():
    return httpx.Client(base_url=BASE_URL, timeout=10)


def test_focus_command(client):
    r = post_update(client, 3001, "/focus")
    assert r.status_code == 200

def test_focus_with_duration(client):
    r = post_update(client, 3002, "/focus 25m")
    assert r.status_code == 200

def test_pause_without_active_session(client):
    r = post_update(client, 3003, "/pause")
    assert r.status_code == 200  # должен вернуть ok, но показать ошибку в боте

def test_stop_without_active_session(client):
    r = post_update(client, 3004, "/stop")
    assert r.status_code == 200

def test_stats_command(client):
    r = post_update(client, 3005, "/stats")
    assert r.status_code == 200
