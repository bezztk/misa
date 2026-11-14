use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskGroup {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum SystemTaskGroupId {
    Personal = 1,
    Shopping = 2,
    School = 3,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskState {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum SystemTaskStateId {
    Open = 1,
    InProgress = 2,
    Completed = 3,
    Cancelled = 4,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct SystemTaskPriority {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum SystemTaskPriorityId {
    Low = 1,
    Normal = 2,
    High = 3,
    Critical = 4,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
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
