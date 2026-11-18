use crate::api::error::{ApiError, ApiResult};
use crate::models::attachment::{Attachment, CreateAttachmentRequest};
use crate::repositories::attachment_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_attachments(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<Json<Vec<Attachment>>> {
    Ok(Json(
        attachment_repository::get_attachments(&pool, node_id).await?,
    ))
}

pub async fn create_attachment(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<CreateAttachmentRequest>,
) -> ApiResult<(StatusCode, Json<Attachment>)> {
    let attachment = attachment_repository::create_attachment(&pool, node_id, &payload)
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok((StatusCode::CREATED, Json(attachment)))
}

pub async fn delete_attachment(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !attachment_repository::delete_attachment(&pool, id).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}
