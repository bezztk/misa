use crate::models::flashcard::{CreateFlashcard, UpdateFlashcard};
use crate::repositories::flashcard_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

#[derive(Serialize)]
pub struct CreateFlashcardResponse {
    pub id: i32,
}

pub async fn create_flashcard(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateFlashcard>,
) -> Result<(StatusCode, Json<CreateFlashcardResponse>), StatusCode> {
    if payload.front.trim().is_empty() || payload.back.trim().is_empty() {
        return Err(StatusCode::BAD_REQUEST);
    }

    match flashcard_repository::create_flashcard(
        &pool,
        payload.document_id,
        &payload.front,
        &payload.back,
        payload.hint.as_deref(),
        payload.explanation.as_deref(),
        payload.retention_levels_id,
        payload.is_enabled,
    )
    .await
    {
        Ok(id) => Ok((StatusCode::CREATED, Json(CreateFlashcardResponse { id }))),
        Err(error) => {
            eprintln!("Failed to create flashcard: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_flashcard(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateFlashcard>,
) -> Result<StatusCode, StatusCode> {
    if payload.front.trim().is_empty() || payload.back.trim().is_empty() {
        return Err(StatusCode::BAD_REQUEST);
    }

    match flashcard_repository::update_flashcard(
        &pool,
        id,
        &payload.front,
        &payload.back,
        payload.hint.as_deref(),
        payload.explanation.as_deref(),
        payload.retention_levels_id,
        payload.is_enabled,
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update flashcard: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn delete_flashcard(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match flashcard_repository::delete_flashcard(&pool, id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete flashcard: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
