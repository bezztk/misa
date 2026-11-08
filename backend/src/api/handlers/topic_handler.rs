use crate::models::topic::{CreateTopic, TopicGraph, UpdateTopic};
use crate::repositories::topic_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

pub async fn get_topic_graph(State(pool): State<PgPool>) -> Result<Json<TopicGraph>, StatusCode> {
    match topic_repository::get_topic_graph(&pool).await {
        Ok(graph) => Ok(Json(graph)),
        Err(error) => {
            eprintln!("Failed to retrieve topic graph: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
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
    if !valid_name(&payload.name, 150)
        || payload.parent_topic_entity_id.is_some() == payload.development_area_id.is_some()
    {
        return Err(StatusCode::BAD_REQUEST);
    }

    match topic_repository::create_topic(
        &pool,
        payload.parent_topic_entity_id,
        payload.development_area_id,
        &payload.name,
    )
    .await
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
    if !valid_name(&payload.name, 150)
        || payload.parent_topic_entity_id.is_some() == payload.development_area_id.is_some()
    {
        return Err(StatusCode::BAD_REQUEST);
    }

    match topic_repository::update_topic(
        &pool,
        entity_id,
        payload.parent_topic_entity_id,
        payload.development_area_id,
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
