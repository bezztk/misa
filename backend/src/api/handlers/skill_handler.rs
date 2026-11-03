use crate::models::skill::{CreateSkill, Skill, UpdateSkill};
use crate::models::skill_state::SkillMetadata;
use crate::repositories::skill_repository;
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

pub async fn get_skill(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<Json<Skill>, StatusCode> {
    match skill_repository::get_skill(&pool, id).await {
        Ok(Some(skill)) => Ok(Json(skill)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to retrieve skill: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn get_skills(State(pool): State<PgPool>) -> Result<Json<Vec<Skill>>, StatusCode> {
    match skill_repository::get_skills(&pool).await {
        Ok(skills) => Ok(Json(skills)),
        Err(error) => {
            eprintln!("Failed to retrieve skills: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

#[derive(Serialize)]
pub struct CreateSkillResponse {
    pub id: i32,
}

pub async fn create_skill(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateSkill>,
) -> Result<(StatusCode, Json<CreateSkillResponse>), StatusCode> {
    if !valid_name(&payload.name) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match skill_repository::create_skill(
        &pool,
        payload.area_id,
        payload.state_id,
        &payload.name,
        payload.description.as_deref(),
    )
    .await
    {
        Ok(id) => Ok((StatusCode::CREATED, Json(CreateSkillResponse { id }))),
        Err(error) => {
            eprintln!("Failed to create skill: {error}");
            if is_foreign_key_error(&error) {
                Err(StatusCode::BAD_REQUEST)
            } else {
                Err(StatusCode::INTERNAL_SERVER_ERROR)
            }
        }
    }
}

pub async fn update_skill(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateSkill>,
) -> Result<StatusCode, StatusCode> {
    if payload.name.is_none() && payload.description.is_none() && payload.state_id.is_none() {
        return Err(StatusCode::BAD_REQUEST);
    }

    if payload
        .name
        .as_deref()
        .is_some_and(|name| !valid_name(name))
    {
        return Err(StatusCode::BAD_REQUEST);
    }

    match skill_repository::update_skill(
        &pool,
        id,
        payload.state_id,
        payload.name.as_deref(),
        payload.description.as_deref(),
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update skill: {error}");
            if is_foreign_key_error(&error) {
                Err(StatusCode::BAD_REQUEST)
            } else {
                Err(StatusCode::INTERNAL_SERVER_ERROR)
            }
        }
    }
}

pub async fn delete_skill(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match skill_repository::delete_skill(&pool, id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete skill: {error}");
            if is_foreign_key_error(&error) {
                Err(StatusCode::CONFLICT)
            } else {
                Err(StatusCode::INTERNAL_SERVER_ERROR)
            }
        }
    }
}

pub async fn get_skill_metadata(
    State(pool): State<PgPool>,
) -> Result<Json<SkillMetadata>, StatusCode> {
    match skill_repository::get_skill_metadata(&pool).await {
        Ok(metadata) => Ok(Json(metadata)),
        Err(error) => {
            eprintln!("Failed to retrieve skill metadata: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
