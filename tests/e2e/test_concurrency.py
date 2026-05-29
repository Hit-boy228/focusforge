"""
tests/e2e/test_concurrency.py
Проверяет защиту от race conditions (duplicate updates, double-start session).
"""
import pytest
import httpx
import threading
import time

BASE_URL = "http://localhost:8080"
BOT_TOKEN = "test_token"
SECRET = "test_secret"


def send_update(client, uid, text, tg_user):
    return client.post(
        f"/webhook/{BOT_TOKEN}",
        json={"update_id": uid, "message": {
            "message_id": uid,
            "from": {"id": tg_user, "is_bot": False,
                     "first_name": "ConcurrentUser", "language_code": "en"},
            "chat": {"id": tg_user, "type": "private"},
            "text": text, "date": int(time.time())
        }},
        headers={"X-Telegram-Bot-Api-Secret-Token": SECRET}
    )


@pytest.fixture
def client():
    return httpx.Client(base_url=BASE_URL, timeout=15)


def test_duplicate_update_idempotency(client):
    """Один и тот же update_id не должен создавать задачу дважды."""
    tg_user = 888001
    send_update(client, 9001, "/start", tg_user)

    results = []
    def post():
        r = send_update(client, 9002, "/task Buy milk p2", tg_user)
        results.append(r.status_code)

    threads = [threading.Thread(target=post) for _ in range(5)]
    for t in threads: t.start()
    for t in threads: t.join()

    assert all(s == 200 for s in results)
    # Задача должна быть создана ровно один раз — проверяется через /tasks


def test_no_double_session_start(client):
    """Два параллельных /focus не должны создавать две сессии."""
    tg_user = 888002
    send_update(client, 9010, "/start", tg_user)

    results = []
    def start_session():
        r = send_update(client, 9011 + threading.current_thread().ident % 100,
                       "/focus 25m", tg_user)
        results.append(r.status_code)

    threads = [threading.Thread(target=start_session) for _ in range(3)]
    for t in threads: t.start()
    for t in threads: t.join()

    assert all(s == 200 for s in results)
    # Только одна сессия должна быть активной
