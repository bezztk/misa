use crate::models::calendar_entry::{CalendarEntry, CreateCalendarEntry, UpdateCalendarEntry};
use crate::repositories::calendar_entry_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

pub async fn get_calendar_entry(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
) -> Result<Json<CalendarEntry>, StatusCode> {
    match calendar_entry_repository::get_calendar_entry(&pool, entity_id).await {
        Ok(Some(entry)) => Ok(Json(entry)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to retrieve calendar entry: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn get_calendar_entries(
    State(pool): State<PgPool>,
) -> Result<Json<Vec<CalendarEntry>>, StatusCode> {
    match calendar_entry_repository::get_calendar_entries(&pool).await {
        Ok(entries) => Ok(Json(entries)),
        Err(error) => {
            eprintln!("Failed to retrieve calendar entries: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

#[derive(Serialize)]
pub struct CreateCalendarEntryResponse {
    pub entity_id: i32,
}

pub async fn create_calendar_entry(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateCalendarEntry>,
) -> Result<(StatusCode, Json<CreateCalendarEntryResponse>), StatusCode> {
    if !valid_name(&payload.name, 150) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match calendar_entry_repository::create_calendar_entry(
        &pool,
        payload.source_entity_id,
        &payload.name,
        payload.note.as_deref(),
        payload.is_reminder,
        payload.is_locked,
        payload.estimated_duration,
        payload.start_at,
        payload.end_at,
    )
    .await
    {
        Ok(entity_id) => Ok((
            StatusCode::CREATED,
            Json(CreateCalendarEntryResponse { entity_id }),
        )),
        Err(error) => {
            eprintln!("Failed to create calendar entry: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_calendar_entry(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
    Json(payload): Json<UpdateCalendarEntry>,
) -> Result<StatusCode, StatusCode> {
    if !valid_name(&payload.name, 150) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match calendar_entry_repository::update_calendar_entry(
        &pool,
        entity_id,
        &payload.name,
        payload.note.as_deref(),
        payload.is_reminder,
        payload.is_locked,
        payload.estimated_duration,
        payload.start_at,
        payload.end_at,
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update calendar entry: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
