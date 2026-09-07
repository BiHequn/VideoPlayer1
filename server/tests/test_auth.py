from fastapi.testclient import TestClient

from app import database
from app.main import app


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