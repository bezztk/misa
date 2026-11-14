use crate::models::system_task::{CreateTaskRequest, TaskListItem, UpdateTaskRequest};
use crate::repositories::task_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

#[derive(Serialize)]
pub struct CreateTaskResponse {
    pub id: i32,
}

pub async fn get_tasks(State(pool): State<PgPool>) -> Result<Json<Vec<TaskListItem>>, StatusCode> {
    match task_repository::get_tasks(&pool).await {
        Ok(tasks) => Ok(Json(tasks)),
        Err(error) => {
            eprintln!("Failed to retrieve tasks: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn create_task(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateTaskRequest>,
) -> Result<(StatusCode, Json<CreateTaskResponse>), StatusCode> {
    if !valid_name(&payload.name, 150) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match task_repository::create_task(
        &pool,
        payload.name.trim(),
        payload.group_id,
        payload.priority_id,
    )
    .await
    {
        Ok(id) => Ok((StatusCode::CREATED, Json(CreateTaskResponse { id }))),
        Err(error) => {
            eprintln!("Failed to create task: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_task(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(mut payload): Json<UpdateTaskRequest>,
) -> Result<StatusCode, StatusCode> {
    if let Some(name) = &payload.name {
        if !valid_name(name, 150) {
            return Err(StatusCode::BAD_REQUEST);
        }

        payload.name = Some(name.trim().to_string());
    }

    match task_repository::update_task(&pool, id, &payload).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update task: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn delete_task(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match task_repository::delete_task(&pool, id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete task: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
