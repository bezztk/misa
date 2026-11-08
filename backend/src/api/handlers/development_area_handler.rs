use crate::models::development_area::{CreateDevelopmentArea, UpdateDevelopmentArea};
use crate::repositories::development_area_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

#[derive(Serialize)]
pub struct CreateDevelopmentAreaResponse {
    pub id: i32,
}

pub async fn create_development_area(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateDevelopmentArea>,
) -> Result<(StatusCode, Json<CreateDevelopmentAreaResponse>), StatusCode> {
    if !valid_name(&payload.name, 50) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match development_area_repository::create_development_area(
        &pool,
        &payload.name,
        payload.background_color.as_deref(),
        payload.banner_image_path.as_deref(),
        payload.icon.as_deref(),
        payload.sort_order,
    )
    .await
    {
        Ok(id) => Ok((
            StatusCode::CREATED,
            Json(CreateDevelopmentAreaResponse { id }),
        )),
        Err(error) => {
            eprintln!("Failed to create development area: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_development_area(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateDevelopmentArea>,
) -> Result<StatusCode, StatusCode> {
    if !valid_name(&payload.name, 50) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match development_area_repository::update_development_area(
        &pool,
        id,
        &payload.name,
        payload.background_color.as_deref(),
        payload.banner_image_path.as_deref(),
        payload.icon.as_deref(),
        payload.sort_order,
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update development area: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
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
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
