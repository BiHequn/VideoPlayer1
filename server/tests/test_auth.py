import base64
import hashlib
import hmac
import json
import time

from fastapi.testclient import TestClient

from app import database
from app.main import app
from app.security import get_access_token_secret


def register_test_user(client: TestClient) -> None:
    response = client.post(
        "/api/auth/register",
        json={
            "username": "testuser",
            "password": "password123",
        },
    )

    assert response.status_code == 201


def test_login_succeeds_with_valid_credentials(tmp_path, monkeypatch) -> None:
    database_path = tmp_path / "users.db"
    monkeypatch.setenv(database.DATABASE_PATH_ENV, str(database_path))

    with TestClient(app) as client:
        register_test_user(client)

        response = client.post(
            "/api/auth/login",
            json={
                "username": "testuser",
                "password": "password123",
            },
        )

    assert response.status_code == 200

    body = response.json()
    assert body["message"] == "login successful"
    assert body["username"] == "testuser"
    assert body["token_type"] == "bearer"
    assert isinstance(body["access_token"], str)
    assert len(body["access_token"]) > 20

    header_part, payload_part, signature_part = body["access_token"].split(".")
    signing_input = f"{header_part}.{payload_part}"
    expected_signature = base64.b64encode(
        hmac.new(
            get_access_token_secret(),
            signing_input.encode("ascii"),
            hashlib.sha256,
        ).digest()
    ).decode("ascii")
    payload = json.loads(base64.b64decode(payload_part))

    assert hmac.compare_digest(signature_part, expected_signature)
    assert payload["uid"] > 0
    assert payload["username"] == "testuser"
    assert payload["exp"] > int(time.time())


def test_login_rejects_wrong_password(tmp_path, monkeypatch) -> None:
    database_path = tmp_path / "users.db"
    monkeypatch.setenv(database.DATABASE_PATH_ENV, str(database_path))

    with TestClient(app) as client:
        register_test_user(client)

        response = client.post(
            "/api/auth/login",
            json={
                "username": "testuser",
                "password": "wrong-password",
            },
        )

    assert response.status_code == 401
    assert response.json() == {
        "detail": "invalid username or password",
    }


def test_login_rejects_unknown_user(tmp_path, monkeypatch) -> None:
    database_path = tmp_path / "users.db"
    monkeypatch.setenv(database.DATABASE_PATH_ENV, str(database_path))

    with TestClient(app) as client:
        response = client.post(
            "/api/auth/login",
            json={
                "username": "unknownuser",
                "password": "password123",
            },
        )

    assert response.status_code == 401
    assert response.json() == {
        "detail": "invalid username or password",
    }
