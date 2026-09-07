from fastapi.testclient import TestClient

from app import database
from app.main import app
from app.security import verify_password


def test_health_check() -> None:
    with TestClient(app) as client:
        response = client.get("/health")

    assert response.status_code == 200
    assert response.json() == {"status": "ok"}


def test_register_creates_user_with_hashed_password(
    tmp_path,
    monkeypatch,
) -> None:
    database_path = tmp_path / "users.db"
    monkeypatch.setenv(database.DATABASE_PATH_ENV, str(database_path))

    with TestClient(app) as client:
        response = client.post(
            "/api/auth/register",
            json={
                "username": "testuser",
                "password": "password123",
            },
        )

    assert response.status_code == 201
    assert response.json() == {
        "message": "registration successful",
        "username": "testuser",
    }

    user = database.get_user_by_username("testuser")

    assert user is not None
    assert user["password_hash"] != "password123"
    assert verify_password("password123", user["password_hash"])


def test_register_rejects_duplicate_username(tmp_path, monkeypatch) -> None:
    database_path = tmp_path / "users.db"
    monkeypatch.setenv(database.DATABASE_PATH_ENV, str(database_path))

    with TestClient(app) as client:
        first_response = client.post(
            "/api/auth/register",
            json={
                "username": "testuser",
                "password": "password123",
            },
        )
        duplicate_response = client.post(
            "/api/auth/register",
            json={
                "username": "testuser",
                "password": "another-password",
            },
        )

    assert first_response.status_code == 201
    assert duplicate_response.status_code == 409
    assert duplicate_response.json() == {
        "detail": "username already exists",
    }
