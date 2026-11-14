use crate::api::error::ApiResult;
use crate::models::domain::DomainNode;
use crate::repositories::domain_repository;
use axum::Json;
use axum::extract::{Path, State};
use sqlx::PgPool;

pub async fn get_domain_nodes(
    State(pool): State<PgPool>,
    Path(domain_id): Path<i32>,
) -> ApiResult<Json<Vec<DomainNode>>> {
    Ok(Json(
        domain_repository::get_domain_nodes(&pool, domain_id).await?,
    ))
}
