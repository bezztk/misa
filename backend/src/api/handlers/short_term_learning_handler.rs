use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

use crate::api::error::{ApiError, ApiResult};
use crate::api::response::IdResponse;
use crate::models::short_term_learning::{
    COMPLETED, CreateShortTermLearningSet, LEARNING, NEW, ShortTermLearningCard,
    ShortTermLearningCardContent, ShortTermLearningSet, UpdateShortTermLearningCard,
    UpdateShortTermLearningState,
};
use crate::repositories::short_term_learning_repository as repository;

pub async fn get_sets(State(pool): State<PgPool>) -> ApiResult<Json<Vec<ShortTermLearningSet>>> {
    Ok(Json(repository::get_sets(&pool).await?))
}

pub async fn create_set(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateShortTermLearningSet>,
) -> ApiResult<(StatusCode, Json<IdResponse>)> {
    let title = payload.title.trim();
    if title.is_empty() {
        return Err(ApiError::BadRequest);
    }
    let id = repository::create_set(&pool, title, payload.bidirectional).await?;
    Ok((StatusCode::CREATED, Json(IdResponse { id })))
}

pub async fn delete_set(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<StatusCode> {
    if !repository::delete_set(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn reset_set(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<StatusCode> {
    if !repository::reset_set(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn get_cards(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<Json<Vec<ShortTermLearningCard>>> {
    Ok(Json(repository::get_cards(&pool, id).await?))
}

pub async fn import_cards(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(cards): Json<Vec<ShortTermLearningCardContent>>,
) -> ApiResult<StatusCode> {
    if !cards.iter().all(ShortTermLearningCardContent::is_valid) {
        return Err(ApiError::BadRequest);
    }
    if !repository::import_cards(&pool, id, &cards).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn update_card(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(card): Json<UpdateShortTermLearningCard>,
) -> ApiResult<StatusCode> {
    if !card.content.is_valid()
        || !matches!(card.front_to_back_state_id, NEW | LEARNING | COMPLETED)
        || card
            .back_to_front_state_id
            .is_some_and(|state| !matches!(state, NEW | LEARNING | COMPLETED))
    {
        return Err(ApiError::BadRequest);
    }
    if !repository::update_card(&pool, id, &card).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn delete_card(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<StatusCode> {
    if !repository::delete_card(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn update_state(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(update): Json<UpdateShortTermLearningState>,
) -> ApiResult<StatusCode> {
    if !matches!(update.state_id, LEARNING | COMPLETED) {
        return Err(ApiError::BadRequest);
    }
    if !repository::update_state(&pool, id, update).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}
