use crate::api::error::{ApiError, ApiResult};
use crate::models::pursuit::{
    CreatePursuitRequest, PursuitDetails, PursuitListItem, UpdatePursuitRequirementsRequest,
};
use crate::repositories::pursuit_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_all_pursuits(State(pool): State<PgPool>) -> ApiResult<Json<Vec<PursuitListItem>>> {
    Ok(Json(pursuit_repository::get_all_pursuits(&pool).await?))
}

pub async fn get_pursuits(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<Json<Vec<PursuitDetails>>> {
    Ok(Json(
        pursuit_repository::get_pursuits(&pool, node_id).await?,
    ))
}

pub async fn create_pursuit(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<CreatePursuitRequest>,
) -> ApiResult<(StatusCode, Json<PursuitDetails>)> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }

    let pursuit = pursuit_repository::create_pursuit(&pool, node_id, payload.name.trim())
        .await?
        .ok_or(ApiError::NotFound)?;
    Ok((StatusCode::CREATED, Json(pursuit)))
}

pub async fn update_requirements(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdatePursuitRequirementsRequest>,
) -> ApiResult<StatusCode> {
    pursuit_repository::update_requirements(&pool, id, &payload)
        .await?
        .ok_or(ApiError::NotFound)?;
    Ok(StatusCode::NO_CONTENT)
}
