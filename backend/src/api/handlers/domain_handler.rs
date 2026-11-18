use crate::api::error::{ApiError, ApiResult};
use crate::models::domain::{
    DomainNode, DomainNodeNameRequest, DomainNodeUpdateRequest, DomainTree,
};
use crate::repositories::domain_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_domain_tree(State(pool): State<PgPool>) -> ApiResult<Json<DomainTree>> {
    Ok(Json(DomainTree {
        domains: domain_repository::get_domains(&pool).await?,
        nodes: domain_repository::get_domain_nodes(&pool).await?,
    }))
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

pub async fn create_root_domain_node(
    State(pool): State<PgPool>,
    Path(domain_id): Path<i32>,
    Json(payload): Json<DomainNodeNameRequest>,
) -> ApiResult<(StatusCode, Json<DomainNode>)> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }

    let node = domain_repository::create_root_domain_node(&pool, domain_id, payload.name.trim())
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok((StatusCode::CREATED, Json(node)))
}

pub async fn update_domain_node(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(mut payload): Json<DomainNodeUpdateRequest>,
) -> ApiResult<Json<DomainNode>> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }

    payload.name = payload.name.trim().to_string();
    let node = domain_repository::update_domain_node(&pool, id, &payload)
        .await?
        .ok_or(ApiError::NotFound)?;

    Ok(Json(node))
}

pub async fn delete_domain_node(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !domain_repository::delete_domain_node(&pool, id).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}
