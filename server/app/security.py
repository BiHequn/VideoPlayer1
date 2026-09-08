import base64
import hashlib
import hmac
import json
import secrets
import time


ALGORITHM = "pbkdf2_sha256"
ITERATIONS = 600_000
SALT_BYTES = 16
ACCESS_TOKEN_TTL_SECONDS = 7200
ACCESS_TOKEN_SECRET = b"VideoPlayer_SecretKey_2026"


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


def create_access_token(user_id: int, username: str) -> str:
    now = int(time.time())
    header = {"alg": "HS256", "typ": "JWT"}
    payload = {
        "uid": user_id,
        "username": username,
        "iat": now,
        "exp": now + ACCESS_TOKEN_TTL_SECONDS,
    }

    header_text = json.dumps(header, separators=(",", ":"), ensure_ascii=True)
    payload_text = json.dumps(payload, separators=(",", ":"), ensure_ascii=True)
    header_part = base64.b64encode(header_text.encode("utf-8")).decode("ascii")
    payload_part = base64.b64encode(payload_text.encode("utf-8")).decode("ascii")
    signing_input = f"{header_part}.{payload_part}"
    signature = hmac.new(
        ACCESS_TOKEN_SECRET,
        signing_input.encode("ascii"),
        hashlib.sha256,
    ).digest()
    signature_part = base64.b64encode(signature).decode("ascii")

    return f"{signing_input}.{signature_part}"
