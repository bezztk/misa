use crate::models::calendar_entry::{CalendarEntry, CreateCalendarEntry, UpdateCalendarEntry};
use crate::repositories::calendar_entry_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use serde::Serialize;
use sqlx::PgPool;

fn valid_title(title: &str) -> bool {
    !title.trim().is_empty() && title.chars().count() <= 150
}

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
    if !valid_title(&payload.title) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match calendar_entry_repository::create_calendar_entry(
        &pool,
        payload.source_entity_id,
        &payload.title,
        payload.note.as_deref(),
        payload.is_reminder,
        payload.planned_start_at,
        payload.planned_end_at,
        payload.actual_start_at,
        payload.actual_end_at,
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
    if !valid_title(&payload.title) {
        return Err(StatusCode::BAD_REQUEST);
    }

    match calendar_entry_repository::update_calendar_entry(
        &pool,
        entity_id,
        payload.source_entity_id,
        &payload.title,
        payload.note.as_deref(),
        payload.is_reminder,
        payload.planned_start_at,
        payload.planned_end_at,
        payload.actual_start_at,
        payload.actual_end_at,
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

pub async fn delete_calendar_entry(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    match calendar_entry_repository::delete_calendar_entry(&pool, entity_id).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to delete calendar entry: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
