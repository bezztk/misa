use crate::api::error::{ApiError, ApiResult};
use crate::models::context::{Context, ContextTitleRequest};
use crate::repositories::context_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_contexts(State(pool): State<PgPool>) -> ApiResult<Json<Vec<Context>>> {
    Ok(Json(context_repository::get_contexts(&pool).await?))
}

pub async fn create_context(
    State(pool): State<PgPool>,
    Json(payload): Json<ContextTitleRequest>,
) -> ApiResult<(StatusCode, Json<Context>)> {
    if !valid_name(&payload.title, 150) {
        return Err(ApiError::BadRequest);
    }

    let context = context_repository::create_context(&pool, payload.title.trim()).await?;
    Ok((StatusCode::CREATED, Json(context)))
}

pub async fn rename_context(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<ContextTitleRequest>,
) -> ApiResult<StatusCode> {
    if !valid_name(&payload.title, 150) {
        return Err(ApiError::BadRequest);
    }
    if !context_repository::rename_context(&pool, id, payload.title.trim()).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn delete_context(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !context_repository::delete_context(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}
