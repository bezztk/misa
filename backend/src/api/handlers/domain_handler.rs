use crate::api::error::{ApiError, ApiResult};
use crate::models::domain::{
    Domain, DomainNode, DomainNodeField, DomainNodeFieldUpdate, DomainNodeFieldUpdateRequest,
    DomainNodeNameRequest, DomainTree, DomainUpdateRequest,
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

pub async fn update_domain(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<DomainUpdateRequest>,
) -> ApiResult<Json<Domain>> {
    let domain = domain_repository::update_domain(
        &pool,
        id,
        payload.color.as_deref(),
        payload.icon.as_deref(),
    )
    .await?
    .ok_or(ApiError::NotFound)?;

    Ok(Json(domain))
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

pub async fn update_domain_node_field(
    State(pool): State<PgPool>,
    Path((id, field_id)): Path<(i32, i32)>,
    Json(payload): Json<DomainNodeFieldUpdateRequest>,
) -> ApiResult<StatusCode> {
    let field = DomainNodeField::try_from(field_id).map_err(|_| ApiError::BadRequest)?;
    let update = match field {
        DomainNodeField::Name => {
            let name = serde_json::from_value::<String>(payload.value)
                .map_err(|_| ApiError::BadRequest)?;
            if !valid_name(&name, 150) {
                return Err(ApiError::BadRequest);
            }
            DomainNodeFieldUpdate::Name(name.trim().to_string())
        }
        DomainNodeField::KnowledgeLevelId => DomainNodeFieldUpdate::KnowledgeLevelId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
        DomainNodeField::CompletenessLevelId => DomainNodeFieldUpdate::CompletenessLevelId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
        DomainNodeField::FlashcardStatusId => DomainNodeFieldUpdate::FlashcardStatusId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
        DomainNodeField::VerificationStatusId => DomainNodeFieldUpdate::VerificationStatusId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
        DomainNodeField::VerificationLevelId => DomainNodeFieldUpdate::VerificationLevelId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
    };

    if !domain_repository::update_domain_node_field(&pool, id, update).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
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
