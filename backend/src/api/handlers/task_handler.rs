use axum::extract::{Path, State};
use axum::http::StatusCode;
use axum::Json;
use serde::Serialize;
use sqlx::PgPool;
use crate::models::task::{CreateTask, Task, UpdateTask};
use crate::repositories::task_repository;

pub async fn get_task(
    State(pool): State<PgPool>,
    Path(id): Path<i64>
) -> Result<Json<Task>, StatusCode> {
    match task_repository::get_task(&pool, id).await {
        Ok(Some(task)) => Ok(Json(task)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(_) => Err(StatusCode::INTERNAL_SERVER_ERROR),
    }
}

#[derive(Serialize)]
pub struct CreateTaskResponse {
    pub id: i64,
}
pub async fn create_task(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateTask>,
) -> Result<(StatusCode, Json<CreateTaskResponse>), StatusCode> {

    match task_repository::create_task(
        &pool,
        &payload.name,
        payload.group_id,
        payload.description.as_deref(),
    ).await {
        Ok(id) => Ok((
            StatusCode::CREATED,
            Json(CreateTaskResponse { id }),
        )),
        Err(error) => {
            eprintln!("Failed to create task: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_task(
    State(pool): State<PgPool>,
    Path(id): Path<i64>,
    Json(payload): Json<UpdateTask>,
) -> Result<StatusCode, StatusCode> {

    if payload.name.is_none() && payload.state_id.is_none() {
        return Err(StatusCode::BAD_REQUEST);
    }

    if let Some(state) = payload.state_id {
        if !(1..=4).contains(&state) {
            return Err(StatusCode::BAD_REQUEST);
        }
    }

    match task_repository::update_task(
        &pool,
        id,
        payload.name.as_deref(),
        payload.state_id,
    ).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update task: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}