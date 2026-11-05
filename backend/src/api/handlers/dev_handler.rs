use crate::dev_seed;
use axum::extract::State;
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn seed(State(pool): State<PgPool>) -> Result<StatusCode, StatusCode> {
    match dev_seed::seed(&pool).await {
        Ok(()) => Ok(StatusCode::NO_CONTENT),
        Err(error) => {
            eprintln!("Failed to seed development data: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
