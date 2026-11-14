use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct CalendarEntry {
    pub id: i32,
    pub systemtask_id: Option<i32>,
    pub routine_execution_id: Option<i32>,
    pub name: String,
    pub note: Option<String>,
    pub start_at: DateTime<Utc>,
    pub end_at: Option<DateTime<Utc>>,
    pub is_locked: bool,
}
