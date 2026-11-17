use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskSessionPreset {
    pub id: i32,
    pub name: String,
    pub planned_duration_minutes: i32,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskSessionState {
    pub id: i32,
    pub name: String,
    pub display_name: String,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
#[expect(
    dead_code,
    reason = "database entity retained for deferred session functionality"
)]
pub struct SystemTaskSession {
    pub id: i32,
    pub systemtask_id: Option<i32>,
    pub routine_execution_id: Option<i32>,
    pub calendar_entry_id: Option<i32>,
    pub note: Option<String>,
    pub state_id: i32,
    pub planned_duration_minutes: Option<i32>,
    pub started_at: DateTime<Utc>,
    pub ended_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
#[expect(
    dead_code,
    reason = "database entity retained for deferred session functionality"
)]
pub struct SystemTaskSessionSegment {
    pub id: i32,
    pub session_id: i32,
    pub started_at: DateTime<Utc>,
    pub ended_at: Option<DateTime<Utc>>,
}
