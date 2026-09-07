import base64
import hashlib
import hmac
import secrets


ALGORITHM = "pbkdf2_sha256"
ITERATIONS = 600_000
SALT_BYTES = 16


def hash_password(password: str) -> str:
    salt = secrets.token_bytes(SALT_BYTES)
    password_hash = hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt,
        ITERATIONS,
    )

    return "$".join(
        (
            ALGORITHM,
            str(ITERATIONS),
            base64.b64encode(salt).decode("ascii"),
            base64.b64encode(password_hash).decode("ascii"),
        )
    )


def verify_password(password: str, stored_hash: str) -> bool:
    algorithm, iterations, salt_text, hash_text = stored_hash.split("$", maxsplit=3)

    if algorithm != ALGORITHM:
        return False

    salt = base64.b64decode(salt_text)
    expected_hash = base64.b64decode(hash_text)
    actual_hash = hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt,
        int(iterations),
    )

    return hmac.compare_digest(actual_hash, expected_hash)
