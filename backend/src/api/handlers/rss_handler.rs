use axum::Json;
use axum::extract::{Path, Query, State};
use axum::http::StatusCode;
use sqlx::PgPool;

use crate::api::error::{ApiError, ApiResult};
use crate::api::response::IdResponse;
use crate::models::rss::{
    AssignRssFeedCategoryRequest, CreateRssFeedRequest, RssCategory, RssEntriesQuery, RssEntry,
    RssFeed, SaveRssCategoryRequest, UpdateRssEntryRequest, UpdateRssFeedRequest,
};
use crate::repositories::rss_repository;
use crate::services::rss_service;
use crate::validation::valid_name;

pub async fn get_feeds(State(pool): State<PgPool>) -> ApiResult<Json<Vec<RssFeed>>> {
    Ok(Json(rss_repository::get_feeds(&pool).await?))
}

pub async fn create_feed(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateRssFeedRequest>,
) -> ApiResult<(StatusCode, Json<IdResponse>)> {
    let url = payload.url.trim();
    let parsed_url = reqwest::Url::parse(url).map_err(|_| ApiError::BadRequest)?;
    if parsed_url.scheme() != "http" && parsed_url.scheme() != "https" {
        return Err(ApiError::BadRequest);
    }

    let feed = rss_repository::create_or_reactivate_feed(&pool, url).await?;
    if feed.enabled {
        let client = rss_service::create_http_client()
            .map_err(|error| ApiError::Internal(Box::new(error)))?;
        if let Err(error) = rss_service::fetch_feed(&pool, &client, &feed).await {
            eprintln!("Initial RSS fetch for {} failed: {error}", feed.url);
        }
    }

    Ok((StatusCode::CREATED, Json(IdResponse { id: feed.id })))
}

pub async fn update_feed(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateRssFeedRequest>,
) -> ApiResult<StatusCode> {
    let url = payload.url.trim();
    let parsed_url = reqwest::Url::parse(url).map_err(|_| ApiError::BadRequest)?;
    if parsed_url.scheme() != "http" && parsed_url.scheme() != "https" {
        return Err(ApiError::BadRequest);
    }

    if !rss_repository::update_feed(&pool, id, url, payload.enabled).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn assign_feed_category(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<AssignRssFeedCategoryRequest>,
) -> ApiResult<StatusCode> {
    if !rss_repository::assign_feed_category(&pool, id, payload.category_id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn refresh_feed(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    let feed = rss_repository::get_active_feed(&pool, id)
        .await?
        .ok_or(ApiError::NotFound)?;
    let client =
        rss_service::create_http_client().map_err(|error| ApiError::Internal(Box::new(error)))?;
    match rss_service::fetch_feed(&pool, &client, &feed).await {
        Ok(()) | Err(rss_service::RssFetchError::AlreadyFetching) => {}
        Err(error) => return Err(ApiError::Internal(Box::new(error))),
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn refresh_all_feeds(State(pool): State<PgPool>) -> ApiResult<StatusCode> {
    let client =
        rss_service::create_http_client().map_err(|error| ApiError::Internal(Box::new(error)))?;
    rss_service::refresh_active_feeds(&pool, &client).await?;
    Ok(StatusCode::NO_CONTENT)
}

pub async fn mark_feed_read(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !rss_repository::mark_feed_read(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn delete_feed(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<StatusCode> {
    if !rss_repository::hide_feed(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn get_entries(
    State(pool): State<PgPool>,
    Query(query): Query<RssEntriesQuery>,
) -> ApiResult<Json<Vec<RssEntry>>> {
    Ok(Json(
        rss_repository::get_entries(&pool, query.feed_id).await?,
    ))
}

pub async fn update_entry(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateRssEntryRequest>,
) -> ApiResult<StatusCode> {
    if !rss_repository::update_entry_read(&pool, id, payload.is_read).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn get_categories(State(pool): State<PgPool>) -> ApiResult<Json<Vec<RssCategory>>> {
    Ok(Json(rss_repository::get_categories(&pool).await?))
}

pub async fn create_category(
    State(pool): State<PgPool>,
    Json(payload): Json<SaveRssCategoryRequest>,
) -> ApiResult<(StatusCode, Json<RssCategory>)> {
    if !valid_name(&payload.name, 100) || payload.color.len() != 7 {
        return Err(ApiError::BadRequest);
    }
    let category = rss_repository::create_category(
        &pool,
        payload.name.trim(),
        payload.icon.as_deref(),
        &payload.color,
    )
    .await?;
    Ok((StatusCode::CREATED, Json(category)))
}

pub async fn update_category(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<SaveRssCategoryRequest>,
) -> ApiResult<Json<RssCategory>> {
    if !valid_name(&payload.name, 100) || payload.color.len() != 7 {
        return Err(ApiError::BadRequest);
    }
    let category = rss_repository::update_category(
        &pool,
        id,
        payload.name.trim(),
        payload.icon.as_deref(),
        &payload.color,
    )
    .await?
    .ok_or(ApiError::NotFound)?;
    Ok(Json(category))
}

pub async fn delete_category(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !rss_repository::delete_category(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}
