"""tests/functional/test_task_flow.py"""
import pytest
import httpx

BASE_URL = "http://localhost:8080"
BOT_TOKEN = "test_token"
SECRET   = "test_secret"
TG_USER  = 222222


def post_update(client, uid, text):
    return client.post(
        f"/webhook/{BOT_TOKEN}",
        json={"update_id": uid, "message": {
            "message_id": uid,
            "from": {"id": TG_USER, "is_bot": False,
                     "first_name": "Alice", "language_code": "en"},
            "chat": {"id": TG_USER, "type": "private"},
            "text": text, "date": 1700000000
        }},
        headers={"X-Telegram-Bot-Api-Secret-Token": SECRET}
    )


@pytest.fixture
def client():
    return httpx.Client(base_url=BASE_URL, timeout=10)


def test_create_task_command(client):
    r = post_update(client, 2001, "/task")
    assert r.status_code == 200

def test_quick_task_input(client):
    r = post_update(client, 2002, "/task Buy milk tomorrow p2 #home")
    assert r.status_code == 200

def test_list_tasks_command(client):
    r = post_update(client, 2003, "/tasks")
    assert r.status_code == 200

def test_today_command(client):
    r = post_update(client, 2004, "/today")
    assert r.status_code == 200

def test_plan_command(client):
    r = post_update(client, 2005, "/plan")
    assert r.status_code == 200
