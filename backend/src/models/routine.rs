use chrono::{DateTime, NaiveDate, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
#[expect(
    dead_code,
    reason = "database entity retained alongside the domain-node response model"
)]
pub struct SystemTaskRoutine {
    pub id: i32,
    pub systemtask_id: i32,
    pub recurrence_rule_set: String,
    pub timezone: String,
    pub is_optional: bool,
    pub is_active: bool,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct DomainNodeRoutine {
    pub id: i32,
    pub systemtask_id: i32,
    pub domain_node_id: i32,
    pub name: String,
    pub recurrence_rule_set: String,
    pub timezone: String,
    pub estimated_duration: Option<i32>,
    pub is_optional: bool,
    pub is_active: bool,
}

#[derive(Debug, Deserialize)]
pub struct SaveDomainNodeRoutineRequest {
    pub name: String,
    pub recurrence_rule_set: String,
    pub timezone: String,
    pub estimated_duration: Option<i32>,
    pub is_optional: bool,
    pub is_active: bool,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskRoutineExecutionState {
    pub id: i32,
    pub name: String,
    pub display_name: String,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
#[expect(
    dead_code,
    reason = "database entity retained for deferred execution management"
)]
pub struct SystemTaskRoutineExecution {
    pub id: i32,
    pub routine_id: i32,
    pub state_id: i32,
    pub is_optional: bool,
    pub occurrence_on: NaiveDate,
    pub scheduled_at: Option<DateTime<Utc>>,
}
