use crate::api::error::{ApiError, ApiResult};
use crate::models::document::{Document, DocumentContentRequest, DocumentVersion};
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

pub async fn delete_document(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !document_repository::delete_document(&pool, node_id).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}

pub async fn get_document_versions(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<Json<Vec<DocumentVersion>>> {
    Ok(Json(
        document_repository::get_document_versions(&pool, node_id).await?,
    ))
}

pub async fn restore_document_version(
    State(pool): State<PgPool>,
    Path((node_id, version)): Path<(i32, i32)>,
) -> ApiResult<Json<Document>> {
    let document = document_repository::restore_document_version(&pool, node_id, version)
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok(Json(document))
}
