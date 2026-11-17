use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

pub const OPEN_SYSTEM_TASK_STATE_ID: i32 = 1;
pub const NORMAL_SYSTEM_TASK_PRIORITY_ID: i32 = 2;

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskGroup {
    pub id: i32,
    pub name: String,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskState {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskPriority {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
#[expect(
    dead_code,
    reason = "database entity retained alongside the task list projection"
)]
pub struct SystemTask {
    pub id: i32,
    pub pursuit_id: Option<i32>,
    pub domain_node_id: Option<i32>,
    pub group_id: Option<i32>,
    pub name: String,
    pub description: Option<String>,
    pub motivation: Option<String>,
    pub desired_outcome: Option<String>,
    pub state_id: i32,
    pub priority_id: i32,
    pub personal_relevance_id: Option<i32>,
    pub lasting_impact_id: Option<i32>,
    pub estimated_effort_minutes: Option<i32>,
    pub due_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct TaskListItem {
    pub id: i32,
    pub name: String,
    pub pursuit_id: Option<i32>,
    pub group_id: Option<i32>,
    pub state_id: i32,
    pub priority_id: i32,
}

#[derive(Debug, Deserialize)]
pub struct CreateTaskRequest {
    pub name: String,
    pub group_id: Option<i32>,
    pub priority_id: i32,
    pub pursuit_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateTaskRequest {
    pub name: Option<String>,
    pub state_id: Option<i32>,
    pub priority_id: Option<i32>,
}
