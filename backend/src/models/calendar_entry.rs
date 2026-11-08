use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct CalendarEntry {
    pub entity_id: i32,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub version: i32,
    pub source_entity_id: Option<i32>,
    pub name: String,
    pub note: Option<String>,
    pub is_reminder: bool,
    pub start_at: DateTime<Utc>,
    pub end_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct CreateCalendarEntry {
    pub source_entity_id: Option<i32>,
    pub name: String,
    pub note: Option<String>,
    pub is_reminder: bool,
    pub start_at: DateTime<Utc>,
    pub end_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateCalendarEntry {
    pub source_entity_id: Option<i32>,
    pub name: String,
    pub note: Option<String>,
    pub is_reminder: bool,
    pub start_at: DateTime<Utc>,
    pub end_at: Option<DateTime<Utc>>,
}
