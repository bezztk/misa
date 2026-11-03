use crate::models::development_category::{
    CreateDevelopmentCategory, DevelopmentCategory, UpdateDevelopmentCategory,
};
use crate::repositories::development_category_repository;
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

pub async fn get_development_category(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<Json<DevelopmentCategory>, StatusCode> {
    match development_category_repository::get_development_category(&pool, id).await {
        Ok(Some(category)) => Ok(Json(category)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to retrieve development category: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn get_development_categories(
    State(pool): State<PgPool>,
) -> Result<Json<Vec<DevelopmentCategory>>, StatusCode> {
    match development_category_repository::get_development_categories(&pool).await {
        Ok(categories) => Ok(Json(categories)),
        Err(error) => {
            eprintln!("Failed to retrieve development categories: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

#[derive(Serialize)]
pub struct CreateDevelopmentCategoryResponse {
    pub id: i32,
}

pub async fn create_development_category(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateDevelopmentCategory>,
) -> Result<(StatusCode, Json<CreateDevelopmentCategoryResponse>), StatusCode> {
    if !valid_name(&payload.name) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match development_category_repository::create_development_category(
        &pool,
        payload.area_id,
        &payload.name,
    )
    .await
    {
        Ok(id) => Ok((
            StatusCode::CREATED,
            Json(CreateDevelopmentCategoryResponse { id }),
        )),
        Err(error) => {
            eprintln!("Failed to create development category: {error}");
            if is_foreign_key_error(&error) {
                Err(StatusCode::BAD_REQUEST)
            } else {
                Err(StatusCode::INTERNAL_SERVER_ERROR)
            }
        }
    }
}

pub async fn update_development_category(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateDevelopmentCategory>,
) -> Result<StatusCode, StatusCode> {
    if payload
        .name
        .as_deref()
        .is_none_or(|name| !valid_name(name))
    {
        return Err(StatusCode::BAD_REQUEST);
    }

    match development_category_repository::update_development_category(
        &pool,
        id,
        payload.name.as_deref(),
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update development category: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn delete_development_category(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match development_category_repository::delete_development_category(&pool, id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete development category: {error}");
            if is_foreign_key_error(&error) {
                Err(StatusCode::CONFLICT)
            } else {
                Err(StatusCode::INTERNAL_SERVER_ERROR)
            }
        }
    }
}
