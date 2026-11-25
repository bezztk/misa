use chrono::{DateTime, Utc};
use serde::Serialize;
use serde_json::Value;

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EntityType {
    DomainNode = 1,
    SystemTask = 2,
    Progression = 3,
    Pursuit = 4,
    Flashcard = 6,
    CalendarEntry = 7,
    Attachment = 8,
    Idea = 9,
    Media = 10,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct EntityFieldVersion {
    pub version: i32,
    pub field_name: String,
    pub old_value: Value,
    pub new_value: Value,
    pub changed_at: DateTime<Utc>,
}
