use crate::api::error::{ApiError, ApiResult};
use crate::models::document::{Document, DocumentContentRequest};
use crate::repositories::document_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn create_document(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<(StatusCode, Json<Document>)> {
    let document = document_repository::create_document(&pool, node_id)
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok((StatusCode::CREATED, Json(document)))
}

pub async fn get_document(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<Json<Document>> {
    let document = document_repository::get_document(&pool, node_id)
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok(Json(document))
}

pub async fn update_document(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<DocumentContentRequest>,
) -> ApiResult<Json<Document>> {
    let document = document_repository::update_document(&pool, node_id, &payload.content)
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok(Json(document))
}
