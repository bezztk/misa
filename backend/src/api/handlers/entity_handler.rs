use crate::repositories::entity_repository;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn archive_entity(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match entity_repository::archive_entity(&pool, entity_id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to archive entity: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn delete_entity(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match entity_repository::delete_entity(&pool, entity_id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete entity: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
