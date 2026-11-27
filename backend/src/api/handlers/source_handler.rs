use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;
use uuid::Uuid;

use crate::api::error::{ApiError, ApiResult};
use crate::models::source::{
    CreateSourceRequest, CreateSourceResponse, Source, UpdateSourceTitleRequest,
};
use crate::repositories::source_repository;
use crate::services::{favicon_service, zotero_client};

pub async fn get_sources(State(pool): State<PgPool>) -> ApiResult<Json<Vec<Source>>> {
    Ok(Json(source_repository::get_sources(&pool).await?))
}

pub async fn create_source(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateSourceRequest>,
) -> ApiResult<(StatusCode, Json<CreateSourceResponse>)> {
    let url = payload.url.trim();
    let parsed_url = reqwest::Url::parse(url).map_err(|_| ApiError::BadRequest)?;
    if !matches!(parsed_url.scheme(), "http" | "https") {
        return Err(ApiError::BadRequest);
    }

    let metadata = zotero_client::translate_web(url).await?;
    // The first item describes the source. The response can also contain child items.
    let item = &metadata[0];
    let resource_type = item["itemType"].as_str().ok_or(ApiError::BadGateway)?;
    let favicon = favicon_service::fetch_favicon(parsed_url).await;
    let id = source_repository::create_source(
        &pool,
        resource_type,
        item["title"].as_str(),
        url,
        item["abstractNote"].as_str(),
        &metadata,
        favicon.as_ref(),
    )
    .await?;
    Ok((StatusCode::CREATED, Json(CreateSourceResponse { id })))
}

pub async fn delete_source(
    State(pool): State<PgPool>,
    Path(id): Path<Uuid>,
) -> ApiResult<StatusCode> {
    if !source_repository::delete_source(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn update_title(
    State(pool): State<PgPool>,
    Path(id): Path<Uuid>,
    Json(payload): Json<UpdateSourceTitleRequest>,
) -> ApiResult<StatusCode> {
    let title = payload.title.trim();
    if title.is_empty() {
        return Err(ApiError::BadRequest);
    }
    if !source_repository::update_title(&pool, id, title).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}
