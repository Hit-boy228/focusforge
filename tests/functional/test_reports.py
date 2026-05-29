"""tests/functional/test_reports.py"""
import pytest
import httpx

BASE_URL = "http://localhost:8080"
BOT_TOKEN = "test_token"
SECRET   = "test_secret"
TG_USER  = 444444


def post_update(client, uid, text):
    return client.post(
        f"/webhook/{BOT_TOKEN}",
        json={"update_id": uid, "message": {
            "message_id": uid,
            "from": {"id": TG_USER, "is_bot": False, "first_name": "Carol",
                     "language_code": "en"},
            "chat": {"id": TG_USER, "type": "private"},
            "text": text, "date": 1700000000
        }},
        headers={"X-Telegram-Bot-Api-Secret-Token": SECRET}
    )


@pytest.fixture
def client():
    return httpx.Client(base_url=BASE_URL, timeout=10)


def test_week_command(client):
    r = post_update(client, 4001, "/week")
    assert r.status_code == 200

def test_stats_command(client):
    r = post_update(client, 4002, "/stats")
    assert r.status_code == 200

def test_review_command(client):
    r = post_update(client, 4003, "/review")
    assert r.status_code == 200
