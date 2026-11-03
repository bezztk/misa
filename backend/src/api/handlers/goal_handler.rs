use crate::models::goal::{CreateGoal, Goal, UpdateGoal};
use crate::repositories::goal_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

fn valid_name(name: &str) -> bool {
    !name.trim().is_empty() && name.chars().count() <= 50
}

fn is_foreign_key_error(error: &sqlx::Error) -> bool {
    error
        .as_database_error()
        .and_then(|error| error.code())
        .as_deref()
        == Some("23503")
}

pub async fn get_goal(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<Json<Goal>, StatusCode> {
    match goal_repository::get_goal(&pool, id).await {
        Ok(Some(goal)) => Ok(Json(goal)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to retrieve goal: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn get_goals(State(pool): State<PgPool>) -> Result<Json<Vec<Goal>>, StatusCode> {
    match goal_repository::get_goals(&pool).await {
        Ok(goals) => Ok(Json(goals)),
        Err(error) => {
            eprintln!("Failed to retrieve goals: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

#[derive(Serialize)]
pub struct CreateGoalResponse {
    pub id: i32,
}

pub async fn create_goal(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateGoal>,
) -> Result<(StatusCode, Json<CreateGoalResponse>), StatusCode> {
    if !valid_name(&payload.name) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match goal_repository::create_goal(
        &pool,
        payload.area_id,
        &payload.name,
        payload.description.as_deref(),
        payload.due_at,
    )
    .await
    {
        Ok(id) => Ok((StatusCode::CREATED, Json(CreateGoalResponse { id }))),
        Err(error) => {
            eprintln!("Failed to create goal: {error}");
            if is_foreign_key_error(&error) {
                Err(StatusCode::BAD_REQUEST)
            } else {
                Err(StatusCode::INTERNAL_SERVER_ERROR)
            }
        }
    }
}

pub async fn update_goal(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateGoal>,
) -> Result<StatusCode, StatusCode> {
    if payload.name.is_none()
        && payload.description.is_none()
        && payload.is_completed.is_none()
        && payload.due_at.is_none()
        && !payload.clear_due_at.unwrap_or(false)
    {
        return Err(StatusCode::BAD_REQUEST);
    }

    if payload
        .name
        .as_deref()
        .is_some_and(|name| !valid_name(name))
    {
        return Err(StatusCode::BAD_REQUEST);
    }

    match goal_repository::update_goal(
        &pool,
        id,
        payload.name.as_deref(),
        payload.description.as_deref(),
        payload.is_completed,
        payload.due_at,
        payload.clear_due_at.unwrap_or(false),
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update goal: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn delete_goal(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match goal_repository::delete_goal(&pool, id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete goal: {error}");
            if is_foreign_key_error(&error) {
                Err(StatusCode::CONFLICT)
            } else {
                Err(StatusCode::INTERNAL_SERVER_ERROR)
            }
        }
    }
}
