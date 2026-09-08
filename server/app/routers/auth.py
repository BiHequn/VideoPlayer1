from fastapi import APIRouter, HTTPException, status

from app import database
from app.schemas import (
    LoginRequest,
    LoginResponse,
    RegisterRequest,
    RegisterResponse,
)
from app.security import create_access_token, hash_password, verify_password


router = APIRouter(prefix="/api/auth", tags=["authentication"])


@router.post(
    "/register",
    response_model=RegisterResponse,
    status_code=status.HTTP_201_CREATED,
)
def register(payload: RegisterRequest) -> RegisterResponse:
    database.initialize_database()

    if database.get_user_by_username(payload.username) is not None:
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail="username already exists",
        )

    database.create_user(
        username=payload.username,
        password_hash=hash_password(payload.password),
    )

    return RegisterResponse(
        message="registration successful",
        username=payload.username,
    )


@router.post(
    "/login",
    response_model=LoginResponse,
)
def login(payload: LoginRequest) -> LoginResponse:
    database.initialize_database()

    user = database.get_user_by_username(payload.username)

    if user is None or not verify_password(
        payload.password,
        user["password_hash"],
    ):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="invalid username or password",
        )

    return LoginResponse(
        message="login successful",
        username=user["username"],
        access_token=create_access_token(user["id"], user["username"]),
    )
