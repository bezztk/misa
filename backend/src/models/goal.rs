use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Goal {
    pub item_id: i32,
    pub area_id: i32,
    pub name: String,
    pub description: Option<String>,
    pub is_completed: bool,
    pub completed_at: Option<DateTime<Utc>>,
    pub due_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct CreateGoal {
    pub area_id: i32,
    pub name: String,
    pub description: Option<String>,
    pub due_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateGoal {
    pub name: Option<String>,
    pub description: Option<String>,
    pub is_completed: Option<bool>,
    pub due_at: Option<DateTime<Utc>>,
}
