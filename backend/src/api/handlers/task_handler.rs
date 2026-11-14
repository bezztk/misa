use crate::api::error::{ApiError, ApiResult};
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

pub async fn get_tasks(State(pool): State<PgPool>) -> ApiResult<Json<Vec<TaskListItem>>> {
    Ok(Json(task_repository::get_tasks(&pool).await?))
}

pub async fn create_task(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateTaskRequest>,
) -> ApiResult<(StatusCode, Json<CreateTaskResponse>)> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }

    let id = task_repository::create_task(
        &pool,
        payload.name.trim(),
        payload.group_id,
        payload.priority_id,
    )
    .await?;

    Ok((StatusCode::CREATED, Json(CreateTaskResponse { id })))
}

pub async fn update_task(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(mut payload): Json<UpdateTaskRequest>,
) -> ApiResult<StatusCode> {
    if let Some(name) = &payload.name {
        if !valid_name(name, 150) {
            return Err(ApiError::BadRequest);
        }

        payload.name = Some(name.trim().to_string());
    }

    if !task_repository::update_task(&pool, id, &payload).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}

pub async fn delete_task(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<StatusCode> {
    if !task_repository::delete_task(&pool, id).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}
