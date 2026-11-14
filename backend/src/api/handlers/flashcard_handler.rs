use crate::models::flashcard::{CreateFlashcard, FlashcardQueueItem, ReviewFlashcardRequest};
use crate::repositories::flashcard_repository::{self, ReviewFlashcardError};
use crate::services::flashcard_review_service;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use chrono::Utc;
use serde::Serialize;
use sqlx::PgPool;

#[derive(Serialize)]
pub struct CreateFlashcardResponse {
    pub id: i32,
}

pub async fn get_due_flashcards(
    State(pool): State<PgPool>,
) -> Result<Json<Vec<FlashcardQueueItem>>, StatusCode> {
    let contexts = match flashcard_repository::get_due_flashcard_contexts(&pool).await {
        Ok(contexts) => contexts,
        Err(error) => {
            eprintln!("Failed to retrieve due flashcards: {error}");
            return Err(StatusCode::INTERNAL_SERVER_ERROR);
        }
    };

    let reviewed_at = Utc::now();
    let mut flashcards = Vec::with_capacity(contexts.len());

    for context in contexts {
        let preview = flashcard_review_service::calculate_review_preview(
            context.review_state_id,
            context.last_rating,
            context.stability,
            context.difficulty,
            context.last_reviewed_at,
            context.desired_retention,
            reviewed_at,
        )
        .map_err(|error| {
            eprintln!("Failed to calculate flashcard preview: {error}");
            StatusCode::INTERNAL_SERVER_ERROR
        })?;

        flashcards.push(FlashcardQueueItem {
            id: context.id,
            front: context.front,
            back: context.back,
            preview,
        });
    }

    Ok(Json(flashcards))
}

pub async fn create_flashcard(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<CreateFlashcard>,
) -> Result<(StatusCode, Json<CreateFlashcardResponse>), StatusCode> {
    if payload.front.trim().is_empty() || payload.back.trim().is_empty() {
        return Err(StatusCode::BAD_REQUEST);
    }

    match flashcard_repository::create_flashcard(
        &pool,
        node_id,
        &payload.front,
        &payload.back,
        payload.retention_class_id,
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

pub async fn review_flashcard(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<ReviewFlashcardRequest>,
) -> Result<StatusCode, StatusCode> {
    if !(1..=4).contains(&payload.rating) || payload.duration_ms < 0 {
        return Err(StatusCode::BAD_REQUEST);
    }

    match flashcard_repository::review_flashcard(&pool, id, payload.rating, payload.duration_ms)
        .await
    {
        Ok(()) => Ok(StatusCode::CREATED),
        Err(ReviewFlashcardError::NotFound) => Err(StatusCode::NOT_FOUND),
        Err(ReviewFlashcardError::Database(error)) => {
            eprintln!("Failed to save flashcard review: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
        Err(ReviewFlashcardError::Calculation(error)) => {
            eprintln!("Failed to calculate flashcard review: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
