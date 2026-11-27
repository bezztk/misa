use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::models::comment::Comment;

pub const OPEN_SYSTEM_TASK_STATE_ID: i32 = 1;

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
    pub domain_node_id: Option<i32>,
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
    pub domain_node_id: Option<i32>,
    pub domain_node_name: Option<String>,
    pub context_ids: Vec<i32>,
    pub state_id: i32,
    pub priority_id: i32,
    pub due_at: Option<DateTime<Utc>>,
    pub planning_count: i64,
    #[sqlx(skip)]
    pub comments: Vec<Comment>,
}

#[derive(Debug, Deserialize)]
pub struct CreateTaskRequest {
    pub name: String,
    pub context_ids: Vec<i32>,
    pub priority_id: i32,
    pub domain_node_id: Option<i32>,
    pub due_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct TaskContextsRequest {
    pub context_ids: Vec<i32>,
}

#[derive(Debug, Deserialize)]
pub struct TaskFieldUpdateRequest {
    pub value: Value,
}

#[repr(i32)]
#[derive(Clone, Copy)]
pub enum TaskField {
    Name = 1,
    StateId = 2,
    PriorityId = 3,
    DueAt = 4,
}

impl TryFrom<i32> for TaskField {
    type Error = ();

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(Self::Name),
            2 => Ok(Self::StateId),
            3 => Ok(Self::PriorityId),
            4 => Ok(Self::DueAt),
            _ => Err(()),
        }
    }
}

pub enum TaskFieldUpdate {
    Name(String),
    StateId(i32),
    PriorityId(i32),
    DueAt(Option<DateTime<Utc>>),
}
