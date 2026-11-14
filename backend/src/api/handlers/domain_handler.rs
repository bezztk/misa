use crate::api::error::{ApiError, ApiResult};
use crate::models::domain::{DomainNode, DomainNodeNameRequest};
use crate::repositories::domain_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_domain_nodes(State(pool): State<PgPool>) -> ApiResult<Json<Vec<DomainNode>>> {
    Ok(Json(domain_repository::get_domain_nodes(&pool).await?))
}

pub async fn create_domain_node(
    State(pool): State<PgPool>,
    Path(parent_id): Path<i32>,
    Json(payload): Json<DomainNodeNameRequest>,
) -> ApiResult<(StatusCode, Json<DomainNode>)> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }

    let node = domain_repository::create_domain_node(&pool, parent_id, payload.name.trim())
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok((StatusCode::CREATED, Json(node)))
}

pub async fn update_domain_node(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<DomainNodeNameRequest>,
) -> ApiResult<Json<DomainNode>> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }

    let node = domain_repository::update_domain_node(&pool, id, payload.name.trim())
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok(Json(node))
}
