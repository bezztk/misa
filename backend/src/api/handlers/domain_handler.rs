use crate::api::error::ApiResult;
use crate::models::domain::DomainNode;
use crate::repositories::domain_repository;
use axum::Json;
use axum::extract::State;
use sqlx::PgPool;

pub async fn get_domain_nodes(State(pool): State<PgPool>) -> ApiResult<Json<Vec<DomainNode>>> {
    Ok(Json(domain_repository::get_domain_nodes(&pool).await?))
}
