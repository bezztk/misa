use crate::api::error::{ApiError, ApiResult};
use crate::api::response::IdResponse;
use crate::models::calendar_entry::{CalendarEntry, SaveCalendarEntryRequest};
use crate::repositories::calendar_entry_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_calendar_entries(
    State(pool): State<PgPool>,
) -> ApiResult<Json<Vec<CalendarEntry>>> {
    Ok(Json(
        calendar_entry_repository::get_calendar_entries(&pool).await?,
    ))
}

pub async fn create_calendar_entry(
    State(pool): State<PgPool>,
    Json(mut payload): Json<SaveCalendarEntryRequest>,
) -> ApiResult<(StatusCode, Json<IdResponse>)> {
    validate(&mut payload)?;
    let id = calendar_entry_repository::create_calendar_entry(&pool, &payload).await?;

    Ok((StatusCode::CREATED, Json(IdResponse { id })))
}

pub async fn update_calendar_entry(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(mut payload): Json<SaveCalendarEntryRequest>,
) -> ApiResult<StatusCode> {
    validate(&mut payload)?;
    if !calendar_entry_repository::update_calendar_entry(&pool, id, &payload).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}

fn validate(payload: &mut SaveCalendarEntryRequest) -> ApiResult<()> {
    if !valid_name(&payload.name, 150)
        || payload.is_reminder != payload.end_at.is_none()
        || payload
            .end_at
            .is_some_and(|end_at| end_at <= payload.start_at)
    {
        return Err(ApiError::BadRequest);
    }

    payload.name = payload.name.trim().to_string();
    Ok(())
}
