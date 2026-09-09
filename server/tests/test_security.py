import pytest

from app.security import (
    MIN_TOKEN_SECRET_LENGTH,
    TOKEN_SECRET_ENV,
    get_access_token_secret,
)


def test_token_secret_is_read_from_environment(monkeypatch) -> None:
    secret = "s" * MIN_TOKEN_SECRET_LENGTH
    monkeypatch.setenv(TOKEN_SECRET_ENV, secret)

    assert get_access_token_secret() == secret.encode("utf-8")


def test_short_token_secret_is_rejected(monkeypatch) -> None:
    monkeypatch.setenv(TOKEN_SECRET_ENV, "too-short")

    with pytest.raises(RuntimeError, match=TOKEN_SECRET_ENV):
        get_access_token_secret()


def test_missing_token_secret_is_rejected(monkeypatch) -> None:
    monkeypatch.delenv(TOKEN_SECRET_ENV, raising=False)

    with pytest.raises(RuntimeError, match=TOKEN_SECRET_ENV):
        get_access_token_secret()
