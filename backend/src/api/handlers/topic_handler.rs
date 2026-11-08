use crate::models::topic::{CreateTopic, UpdateTopic};
use crate::repositories::topic_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

fn valid_name(name: &str) -> bool {
    !name.trim().is_empty() && name.chars().count() <= 150
}

pub async fn get_topic_graph() -> StatusCode {
    StatusCode::NOT_IMPLEMENTED
}

pub async fn get_topic_graph_by_id(Path(_entity_id): Path<i32>) -> StatusCode {
    StatusCode::NOT_IMPLEMENTED
}

#[derive(Serialize)]
pub struct CreateTopicResponse {
    pub entity_id: i32,
}

pub async fn create_topic(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateTopic>,
) -> Result<(StatusCode, Json<CreateTopicResponse>), StatusCode> {
    if !valid_name(&payload.name) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match topic_repository::create_topic(&pool, &payload.name, payload.understanding_level_id).await
    {
        Ok(entity_id) => Ok((StatusCode::CREATED, Json(CreateTopicResponse { entity_id }))),
        Err(error) => {
            eprintln!("Failed to create topic: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_topic(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
    Json(payload): Json<UpdateTopic>,
) -> Result<StatusCode, StatusCode> {
    if !valid_name(&payload.name) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match topic_repository::update_topic(
        &pool,
        entity_id,
        &payload.name,
        payload.understanding_level_id,
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update topic: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn delete_topic(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match topic_repository::delete_topic(&pool, entity_id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete topic: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
