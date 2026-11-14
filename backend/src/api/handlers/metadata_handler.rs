use crate::api::error::ApiResult;
use crate::models::metadata::Metadata;
use crate::repositories::metadata_repository;
use axum::Json;
use axum::extract::State;
use sqlx::PgPool;

pub async fn get_metadata(State(pool): State<PgPool>) -> ApiResult<Json<Metadata>> {
    Ok(Json(metadata_repository::get_metadata(&pool).await?))
}
