use crate::models::flashcard::{
    CreateFlashcard, CreateFlashcardReview, Flashcard, FlashcardReviewPreview, UpdateFlashcard,
};
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
) -> Result<Json<Vec<Flashcard>>, StatusCode> {
    match flashcard_repository::get_due_flashcards(&pool).await {
        Ok(flashcards) => Ok(Json(flashcards)),
        Err(error) => {
            eprintln!("Failed to retrieve due flashcards: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn preview_flashcard(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<Json<FlashcardReviewPreview>, StatusCode> {
    let context = match flashcard_repository::get_flashcard_review_context(&pool, id).await {
        Ok(Some(context)) => context,
        Ok(None) => return Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to retrieve flashcard preview data: {error}");
            return Err(StatusCode::INTERNAL_SERVER_ERROR);
        }
    };

    match flashcard_review_service::calculate_review_preview(
        context.state,
        context.learning_step,
        context.stability,
        context.difficulty,
        context.last_reviewed_at,
        context.desired_retention,
        Utc::now(),
    ) {
        Ok(preview) => Ok(Json(preview)),
        Err(error) => {
            eprintln!("Failed to calculate flashcard preview: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
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

pub async fn review_flashcard(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<CreateFlashcardReview>,
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
