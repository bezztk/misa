use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Task {
    pub entity_id: i32,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub version: i32,
    pub group_id: i32,
    pub name: String,
    pub description: Option<String>,
    pub state_id: i32,
    pub priority_id: i32,
    pub due_at: Option<DateTime<Utc>>,
    pub completed_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct CreateTask {
    pub group_id: i32,
    pub name: String,
    pub description: Option<String>,
    pub priority_id: i32,
    pub due_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateTask {
    pub group_id: i32,
    pub name: String,
    pub description: Option<String>,
    pub state_id: i32,
    pub priority_id: i32,
    pub due_at: Option<DateTime<Utc>>,
}
