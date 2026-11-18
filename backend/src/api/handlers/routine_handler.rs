use crate::api::error::ApiResult;
use crate::services::routine_execution_service;
use axum::Json;
use axum::extract::State;
use serde::Serialize;
use sqlx::PgPool;

#[derive(Serialize)]
pub struct GenerateExecutionsResponse {
    pub generated: usize,
}

pub async fn generate_executions(
    State(pool): State<PgPool>,
) -> ApiResult<Json<GenerateExecutionsResponse>> {
    Ok(Json(GenerateExecutionsResponse {
        generated: routine_execution_service::generate_next_executions(&pool).await?,
    }))
}
