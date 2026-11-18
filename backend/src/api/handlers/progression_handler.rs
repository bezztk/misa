use crate::api::error::{ApiError, ApiResult};
use crate::models::progression::{
    AssignPursuitRequest, CreateProgressionRequest, CreateProgressionStageRequest,
    ProgressionField, ProgressionFieldUpdate, ProgressionFieldUpdateRequest, ProgressionOverview,
    ProgressionPursuit,
};
use crate::repositories::progression_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_progressions(State(pool): State<PgPool>) -> ApiResult<Json<ProgressionOverview>> {
    Ok(Json(ProgressionOverview {
        progressions: progression_repository::get_progressions(&pool).await?,
        pursuits: progression_repository::get_pursuits(&pool).await?,
    }))
}

pub async fn get_available_pursuits(
    State(pool): State<PgPool>,
) -> ApiResult<Json<Vec<ProgressionPursuit>>> {
    Ok(Json(
        progression_repository::get_available_pursuits(&pool).await?,
    ))
}

pub async fn create_progression(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateProgressionRequest>,
) -> ApiResult<StatusCode> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }
    progression_repository::create_progression(&pool, payload.name.trim()).await?;
    Ok(StatusCode::CREATED)
}

pub async fn update_progression_field(
    State(pool): State<PgPool>,
    Path((id, field_id)): Path<(i32, i32)>,
    Json(payload): Json<ProgressionFieldUpdateRequest>,
) -> ApiResult<StatusCode> {
    let field = ProgressionField::try_from(field_id).map_err(|_| ApiError::BadRequest)?;
    let update = match field {
        ProgressionField::Name => {
            let name = serde_json::from_value::<String>(payload.value)
                .map_err(|_| ApiError::BadRequest)?;
            if !valid_name(&name, 150) {
                return Err(ApiError::BadRequest);
            }
            ProgressionFieldUpdate::Name(name.trim().to_string())
        }
        ProgressionField::PersonalRelevanceId => ProgressionFieldUpdate::PersonalRelevanceId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
        ProgressionField::LastingImpactId => ProgressionFieldUpdate::LastingImpactId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
    };

    if !progression_repository::update_progression_field(&pool, id, update).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}

pub async fn create_stage(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<CreateProgressionStageRequest>,
) -> ApiResult<StatusCode> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }
    if !progression_repository::create_stage(&pool, id, payload.name.trim()).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::CREATED)
}

pub async fn assign_pursuit(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<AssignPursuitRequest>,
) -> ApiResult<StatusCode> {
    if !progression_repository::assign_pursuit(&pool, id, payload.pursuit_id).await? {
        return Err(ApiError::BadRequest);
    }
    Ok(StatusCode::NO_CONTENT)
}
