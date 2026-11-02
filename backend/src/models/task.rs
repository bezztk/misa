use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Task {
    pub item_id: i64,
    pub group_id: Option<i32>,
    pub state_id: i32,
    pub name: String,
    pub description: Option<String>,
    pub completed_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct CreateTask {
    pub name: String,
    pub group_id: Option<i32>,
    pub description: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateTask {
    pub name: Option<String>,
    pub state_id: Option<i32>,
    pub group_id: Option<i32>,
    pub description: Option<String>,
}

#[derive(Debug, sqlx::FromRow, serde::Serialize)]
pub struct TaskDetails {
    pub item_id: i64,
    pub item_type: i32,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub is_archived: bool,
    pub version: i32,

    pub group_id: Option<i32>,
    pub state_id: i32,
    pub name: String,
    pub description: Option<String>,
    pub completed_at: Option<DateTime<Utc>>,
}
