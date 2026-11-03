use crate::models::development_area::{
    CreateDevelopmentArea, DevelopmentArea, UpdateDevelopmentArea,
};
use crate::repositories::development_area_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

fn valid_name(name: &str) -> bool {
    !name.trim().is_empty() && name.chars().count() <= 50
}

fn database_error_status(error: &sqlx::Error) -> StatusCode {
    match error
        .as_database_error()
        .and_then(|error| error.code())
        .as_deref()
    {
        Some("23503" | "23505") => StatusCode::CONFLICT,
        _ => StatusCode::INTERNAL_SERVER_ERROR,
    }
}

pub async fn get_development_area(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<Json<DevelopmentArea>, StatusCode> {
    match development_area_repository::get_development_area(&pool, id).await {
        Ok(Some(area)) => Ok(Json(area)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to retrieve development area: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn get_development_areas(
    State(pool): State<PgPool>,
) -> Result<Json<Vec<DevelopmentArea>>, StatusCode> {
    match development_area_repository::get_development_areas(&pool).await {
        Ok(areas) => Ok(Json(areas)),
        Err(error) => {
            eprintln!("Failed to retrieve development areas: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

#[derive(Serialize)]
pub struct CreateDevelopmentAreaResponse {
    pub id: i32,
}

pub async fn create_development_area(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateDevelopmentArea>,
) -> Result<(StatusCode, Json<CreateDevelopmentAreaResponse>), StatusCode> {
    if !valid_name(&payload.name) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match development_area_repository::create_development_area(
        &pool,
        &payload.name,
        payload.description.as_deref(),
    )
    .await
    {
        Ok(id) => Ok((
            StatusCode::CREATED,
            Json(CreateDevelopmentAreaResponse { id }),
        )),
        Err(error) => {
            eprintln!("Failed to create development area: {error}");
            Err(database_error_status(&error))
        }
    }
}

pub async fn update_development_area(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateDevelopmentArea>,
) -> Result<StatusCode, StatusCode> {
    if payload.name.is_none() && payload.description.is_none() {
        return Err(StatusCode::BAD_REQUEST);
    }

    if payload
        .name
        .as_deref()
        .is_some_and(|name| !valid_name(name))
    {
        return Err(StatusCode::BAD_REQUEST);
    }

    match development_area_repository::update_development_area(
        &pool,
        id,
        payload.name.as_deref(),
        payload.description.as_deref(),
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update development area: {error}");
            Err(database_error_status(&error))
        }
    }
}

pub async fn delete_development_area(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match development_area_repository::delete_development_area(&pool, id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete development area: {error}");
            Err(database_error_status(&error))
        }
    }
}
