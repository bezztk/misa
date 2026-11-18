use crate::api::error::{ApiError, ApiResult};
use crate::models::flashcard::{
    CreateFlashcardRequest, EditableFlashcard, FlashcardField, FlashcardFieldUpdate,
    FlashcardFieldUpdateRequest, FlashcardQueueItem, ReviewFlashcardRequest,
};
use crate::repositories::flashcard_repository;
use crate::services::flashcard_review_service;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_due_flashcards(
    State(pool): State<PgPool>,
) -> ApiResult<Json<Vec<FlashcardQueueItem>>> {
    Ok(Json(
        flashcard_review_service::get_due_flashcards(&pool).await?,
    ))
}

pub async fn create_flashcard(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<CreateFlashcardRequest>,
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

pub async fn update_flashcard_field(
    State(pool): State<PgPool>,
    Path((id, field_id)): Path<(i32, i32)>,
    Json(payload): Json<FlashcardFieldUpdateRequest>,
) -> ApiResult<StatusCode> {
    let field = FlashcardField::try_from(field_id).map_err(|_| ApiError::BadRequest)?;
    let value =
        serde_json::from_value::<String>(payload.value).map_err(|_| ApiError::BadRequest)?;
    if value.trim().is_empty() {
        return Err(ApiError::BadRequest);
    }
    let update = match field {
        FlashcardField::Front => FlashcardFieldUpdate::Front(value.trim().to_string()),
        FlashcardField::Back => FlashcardFieldUpdate::Back(value.trim().to_string()),
    };

    if !flashcard_repository::update_flashcard_field(&pool, id, update).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
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

    flashcard_review_service::review_flashcard(&pool, id, payload.rating, payload.duration_ms)
        .await?;

    Ok(StatusCode::CREATED)
}
