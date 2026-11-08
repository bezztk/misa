use crate::models::task::{CreateTask, Task, UpdateTask};
use crate::repositories::task_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

async fn validate_state(pool: &PgPool, state_id: i32) -> Result<(), StatusCode> {
    match task_repository::task_state_exists(pool, state_id).await {
        Ok(true) => Ok(()),
        Ok(false) => Err(StatusCode::BAD_REQUEST),
        Err(error) => {
            eprintln!("Failed to validate task state: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

async fn validate_group(pool: &PgPool, group_id: i32) -> Result<(), StatusCode> {
    match task_repository::task_group_exists(pool, group_id).await {
        Ok(true) => Ok(()),
        Ok(false) => Err(StatusCode::BAD_REQUEST),
        Err(error) => {
            eprintln!("Failed to validate task group: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

async fn validate_priority(pool: &PgPool, priority_id: i32) -> Result<(), StatusCode> {
    match task_repository::task_priority_exists(pool, priority_id).await {
        Ok(true) => Ok(()),
        Ok(false) => Err(StatusCode::BAD_REQUEST),
        Err(error) => {
            eprintln!("Failed to validate task priority: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn get_task(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
) -> Result<Json<Task>, StatusCode> {
    match task_repository::get_task(&pool, entity_id).await {
        Ok(Some(task)) => Ok(Json(task)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(_) => Err(StatusCode::INTERNAL_SERVER_ERROR),
    }
}

pub async fn get_tasks(State(pool): State<PgPool>) -> Result<Json<Vec<Task>>, StatusCode> {
    match task_repository::get_tasks(&pool).await {
        Ok(tasks) => Ok(Json(tasks)),
        Err(error) => {
            eprintln!("Failed to retrieve tasks: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

#[derive(Serialize)]
pub struct CreateTaskResponse {
    pub entity_id: i32,
}
pub async fn create_task(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateTask>,
) -> Result<(StatusCode, Json<CreateTaskResponse>), StatusCode> {
    if !valid_name(&payload.name, 150) {
        return Err(StatusCode::BAD_REQUEST);
    }

    validate_group(&pool, payload.group_id).await?;
    validate_priority(&pool, payload.priority_id).await?;

    match task_repository::create_task(
        &pool,
        payload.group_id,
        &payload.name,
        payload.description.as_deref(),
        payload.priority_id,
        payload.due_at,
    )
    .await
    {
        Ok(entity_id) => Ok((StatusCode::CREATED, Json(CreateTaskResponse { entity_id }))),
        Err(error) => {
            eprintln!("Failed to create task: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_task(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
    Json(payload): Json<UpdateTask>,
) -> Result<StatusCode, StatusCode> {
    if !valid_name(&payload.name, 150) {
        return Err(StatusCode::BAD_REQUEST);
    }

    validate_state(&pool, payload.state_id).await?;
    validate_group(&pool, payload.group_id).await?;
    validate_priority(&pool, payload.priority_id).await?;

    match task_repository::update_task(
        &pool,
        entity_id,
        payload.group_id,
        &payload.name,
        payload.description.as_deref(),
        payload.state_id,
        payload.priority_id,
        payload.due_at,
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update task: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
