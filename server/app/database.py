import os
import sqlite3
from pathlib import Path


DATABASE_PATH_ENV = "VIDEOPLAYER_DATABASE_PATH"
DEFAULT_DATABASE_PATH = (
    Path(__file__).resolve().parent.parent / "data" / "videoplayer_users.db"
)


def get_database_path() -> Path:
    configured_path = os.getenv(DATABASE_PATH_ENV)

    if configured_path:
        return Path(configured_path)

    return DEFAULT_DATABASE_PATH


def get_connection() -> sqlite3.Connection:
    database_path = get_database_path()
    database_path.parent.mkdir(parents=True, exist_ok=True)

    connection = sqlite3.connect(database_path)
    connection.row_factory = sqlite3.Row
    return connection


def initialize_database() -> None:
    with get_connection() as connection:
        connection.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL UNIQUE,
                password_hash TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            )
            """
        )


def get_user_by_username(username: str) -> sqlite3.Row | None:
    with get_connection() as connection:
        return connection.execute(
            "SELECT id, username, password_hash, created_at FROM users WHERE username = ?",
            (username,),
        ).fetchone()


def create_user(username: str, password_hash: str) -> None:
    with get_connection() as connection:
        connection.execute(
            "INSERT INTO users (username, password_hash) VALUES (?, ?)",
            (username, password_hash),
        )
