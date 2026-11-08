use crate::models::document::UpdateDocument;
use crate::repositories::document_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn update_document(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
    Json(payload): Json<UpdateDocument>,
) -> Result<StatusCode, StatusCode> {
    match document_repository::update_document(
        &pool,
        entity_id,
        &payload.content,
        payload.completeness_level_id,
        payload.flashcard_status_id,
        payload.verification_status_id,
        payload.verification_level_id,
        payload.last_verified_at,
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update document: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
