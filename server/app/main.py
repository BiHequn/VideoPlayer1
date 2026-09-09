from fastapi import FastAPI

from app import database
from app.routers.auth import router as auth_router
from app.security import get_access_token_secret


app = FastAPI(
    title="VideoPlayer1 API",
    version="0.2.0",
)

app.include_router(auth_router)


@app.on_event("startup")
def startup() -> None:
    get_access_token_secret()
    database.initialize_database()


@app.get("/health")
def health_check() -> dict[str, str]:
    return {"status": "ok"}
