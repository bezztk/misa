use crate::api::error::{ApiError, ApiResult};
use crate::models::calendar_entry::{CalendarEntry, CreateCalendarEntryRequest};
use crate::repositories::calendar_entry_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::State;
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

#[derive(Serialize)]
pub struct CreateCalendarEntryResponse {
    pub id: i32,
}

pub async fn get_calendar_entries(
    State(pool): State<PgPool>,
) -> ApiResult<Json<Vec<CalendarEntry>>> {
    Ok(Json(
        calendar_entry_repository::get_calendar_entries(&pool).await?,
    ))
}

pub async fn create_calendar_entry(
    State(pool): State<PgPool>,
    Json(mut payload): Json<CreateCalendarEntryRequest>,
) -> ApiResult<(StatusCode, Json<CreateCalendarEntryResponse>)> {
    if !valid_name(&payload.name, 150)
        || payload.is_reminder != payload.end_at.is_none()
        || payload
            .end_at
            .is_some_and(|end_at| end_at <= payload.start_at)
    {
        return Err(ApiError::BadRequest);
    }

    payload.name = payload.name.trim().to_string();
    let id = calendar_entry_repository::create_calendar_entry(&pool, &payload).await?;

    Ok((
        StatusCode::CREATED,
        Json(CreateCalendarEntryResponse { id }),
    ))
}
