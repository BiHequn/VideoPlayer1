from fastapi import APIRouter, HTTPException, status

from app import database
from app.schemas import RegisterRequest, RegisterResponse
from app.security import hash_password


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
