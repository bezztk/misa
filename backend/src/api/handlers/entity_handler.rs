use crate::api::error::ApiResult;
use crate::models::entity::EntityFieldVersion;
use crate::repositories::entity_repository;
use axum::Json;
use axum::extract::{Path, State};
use sqlx::PgPool;

pub async fn get_field_versions(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
) -> ApiResult<Json<Vec<EntityFieldVersion>>> {
    Ok(Json(
        entity_repository::get_field_versions(&pool, entity_id).await?,
    ))
}
