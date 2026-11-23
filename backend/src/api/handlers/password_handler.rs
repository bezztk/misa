use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

use crate::api::error::{ApiError, ApiResult};
use crate::models::password::{
    PasswordCategory, PasswordEntry, SavePasswordCategoryRequest, SavePasswordEntryRequest,
};
use crate::repositories::password_repository;
use crate::validation::valid_name;

pub async fn get_entries(State(pool): State<PgPool>) -> ApiResult<Json<Vec<PasswordEntry>>> {
    Ok(Json(password_repository::get_entries(&pool).await?))
}

pub async fn create_entry(
    State(pool): State<PgPool>,
    Json(payload): Json<SavePasswordEntryRequest>,
) -> ApiResult<(StatusCode, Json<PasswordEntry>)> {
    let entry = password_repository::create_entry(
        &pool,
        payload.category_id,
        &payload.ciphertext,
        &payload.salt,
        &payload.nonce,
    )
    .await?;
    Ok((StatusCode::CREATED, Json(entry)))
}

pub async fn update_entry(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<SavePasswordEntryRequest>,
) -> ApiResult<Json<PasswordEntry>> {
    let entry = password_repository::update_entry(
        &pool,
        id,
        payload.category_id,
        &payload.ciphertext,
        &payload.salt,
        &payload.nonce,
    )
    .await?
    .ok_or(ApiError::NotFound)?;
    Ok(Json(entry))
}

pub async fn delete_entry(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !password_repository::delete_entry(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn get_categories(State(pool): State<PgPool>) -> ApiResult<Json<Vec<PasswordCategory>>> {
    Ok(Json(password_repository::get_categories(&pool).await?))
}

pub async fn create_category(
    State(pool): State<PgPool>,
    Json(payload): Json<SavePasswordCategoryRequest>,
) -> ApiResult<(StatusCode, Json<PasswordCategory>)> {
    if !valid_name(&payload.name, 100) {
        return Err(ApiError::BadRequest);
    }
    let category = password_repository::create_category(&pool, payload.name.trim()).await?;
    Ok((StatusCode::CREATED, Json(category)))
}

pub async fn update_category(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<SavePasswordCategoryRequest>,
) -> ApiResult<Json<PasswordCategory>> {
    if !valid_name(&payload.name, 100) {
        return Err(ApiError::BadRequest);
    }
    let category = password_repository::update_category(&pool, id, payload.name.trim())
        .await?
        .ok_or(ApiError::NotFound)?;
    Ok(Json(category))
}

pub async fn delete_category(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !password_repository::delete_category(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}
