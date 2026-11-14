use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskRoutine {
    pub id: i32,
    pub systemtask_id: i32,
    pub recurrence_rule_set: String,
    pub timezone: String,
    pub is_optional: bool,
    pub is_active: bool,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskRoutineExecutionState {
    pub id: i32,
    pub name: String,
    pub display_name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum SystemTaskRoutineExecutionStateId {
    Pending = 1,
    Completed = 2,
    Skipped = 3,
    Expired = 4,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskRoutineExecution {
    pub id: i32,
    pub routine_id: i32,
    pub state_id: i32,
    pub is_optional: bool,
    pub occurrence_at: DateTime<Utc>,
}
