import pytest

from app.security import TOKEN_SECRET_ENV


@pytest.fixture(autouse=True)
def configure_token_secret(monkeypatch) -> None:
    monkeypatch.setenv(
        TOKEN_SECRET_ENV,
        "test-only-token-secret-with-at-least-32-characters",
    )
