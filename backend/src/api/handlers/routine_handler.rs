use crate::api::error::{ApiError, ApiResult};
use crate::models::routine::{DomainNodeRoutine, SaveDomainNodeRoutineRequest};
use crate::repositories::routine_repository::{self, CreateRoutineResult};
use crate::services::routine_execution_service;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
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

pub async fn get_domain_node_routine(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<Json<DomainNodeRoutine>> {
    routine_repository::get_domain_node_routine(&pool, node_id)
        .await?
        .map(Json)
        .ok_or(ApiError::NotFound)
}

pub async fn create_domain_node_routine(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<SaveDomainNodeRoutineRequest>,
) -> ApiResult<(StatusCode, Json<DomainNodeRoutine>)> {
    validate(&payload)?;

    match routine_repository::create_domain_node_routine(&pool, node_id, &payload).await? {
        CreateRoutineResult::Created(routine) => Ok((StatusCode::CREATED, Json(routine))),
        CreateRoutineResult::NodeNotFound => Err(ApiError::NotFound),
        CreateRoutineResult::RoutineExists => Err(ApiError::Conflict),
    }
}

pub async fn update_domain_node_routine(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<SaveDomainNodeRoutineRequest>,
) -> ApiResult<Json<DomainNodeRoutine>> {
    validate(&payload)?;

    routine_repository::update_domain_node_routine(&pool, node_id, &payload)
        .await?
        .map(Json)
        .ok_or(ApiError::NotFound)
}

fn validate(payload: &SaveDomainNodeRoutineRequest) -> ApiResult<()> {
    if !valid_name(&payload.name, 150)
        || payload.timezone != "Europe/Berlin"
        || payload
            .estimated_duration
            .is_some_and(|duration| duration <= 0)
        || !routine_execution_service::valid_recurrence_rule_set(&payload.recurrence_rule_set)
    {
        return Err(ApiError::BadRequest);
    }
    Ok(())
}
