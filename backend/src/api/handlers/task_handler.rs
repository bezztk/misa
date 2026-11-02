use axum::extract::{Path, State};
use axum::http::StatusCode;
use axum::Json;

use sqlx::PgPool;
use crate::models::task::Task;
use crate::repositories::task_repository;

pub async fn get_task(State(pool): State<PgPool>, Path(id): Path<i64>) -> Result<Json<Task>, StatusCode> {
    match task_repository::get_task(&pool, id).await {
        Ok(Some(task)) => Ok(Json(task)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(_) => Err(StatusCode::INTERNAL_SERVER_ERROR),
    }
}