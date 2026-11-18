use crate::api::error::{ApiError, ApiResult};
use crate::models::comment::{Comment, CreateCommentRequest, UpdateCommentRequest};
use crate::repositories::comment_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn create_comment(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
    Json(payload): Json<CreateCommentRequest>,
) -> ApiResult<(StatusCode, Json<Comment>)> {
    let comment = comment_repository::create_comment(&pool, entity_id, &payload.content).await?;

    Ok((StatusCode::CREATED, Json(comment)))
}

pub async fn update_comment(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateCommentRequest>,
) -> ApiResult<Json<Comment>> {
    let comment = comment_repository::update_comment(&pool, id, &payload.content)
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok(Json(comment))
}

pub async fn delete_comment(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !comment_repository::delete_comment(&pool, id).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}
