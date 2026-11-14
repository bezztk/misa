use crate::api::error::{ApiError, ApiResult};
use crate::models::flashcard::{
    CreateFlashcard, EditableFlashcard, FlashcardQueueItem, ReviewFlashcardRequest, UpdateFlashcard,
};
use crate::repositories::flashcard_repository;
use crate::services::flashcard_review_service;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use chrono::Utc;
use sqlx::PgPool;

pub async fn get_due_flashcards(
    State(pool): State<PgPool>,
) -> ApiResult<Json<Vec<FlashcardQueueItem>>> {
    let contexts = flashcard_repository::get_due_flashcard_contexts(&pool).await?;

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
        )?;

        flashcards.push(FlashcardQueueItem {
            id: context.id,
            front: context.front,
            back: context.back,
            review_state_id: context.review_state_id,
            retention_class_id: context.retention_class_id,
            learning_step: i16::from(
                matches!(context.review_state_id, 2 | 4) && context.last_rating == Some(3),
            ),
            stability: context.stability,
            difficulty: context.difficulty,
            preview,
        });
    }

    Ok(Json(flashcards))
}

pub async fn create_flashcard(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<CreateFlashcard>,
) -> ApiResult<(StatusCode, Json<EditableFlashcard>)> {
    if payload.front.trim().is_empty() || payload.back.trim().is_empty() {
        return Err(ApiError::BadRequest);
    }

    let flashcard =
        flashcard_repository::create_flashcard(&pool, node_id, &payload.front, &payload.back)
            .await?;

    Ok((StatusCode::CREATED, Json(flashcard)))
}

pub async fn get_flashcards(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<Json<Vec<EditableFlashcard>>> {
    Ok(Json(
        flashcard_repository::get_flashcards(&pool, node_id).await?,
    ))
}

pub async fn update_flashcard(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateFlashcard>,
) -> ApiResult<Json<EditableFlashcard>> {
    if payload.front.trim().is_empty() || payload.back.trim().is_empty() {
        return Err(ApiError::BadRequest);
    }

    let flashcard =
        flashcard_repository::update_flashcard(&pool, id, &payload.front, &payload.back)
            .await?
            .ok_or(ApiError::NotFound)?;

    Ok(Json(flashcard))
}

pub async fn delete_flashcard(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !flashcard_repository::delete_flashcard(&pool, id).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}

pub async fn review_flashcard(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<ReviewFlashcardRequest>,
) -> ApiResult<StatusCode> {
    if !(1..=4).contains(&payload.rating) || payload.duration_ms < 0 {
        return Err(ApiError::BadRequest);
    }

    flashcard_repository::review_flashcard(&pool, id, payload.rating, payload.duration_ms).await?;

    Ok(StatusCode::CREATED)
}
