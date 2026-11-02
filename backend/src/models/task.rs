use chrono::{DateTime, Utc};
use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Task {
    pub item_id: i64,
    pub group_id: Option<i32>,
    pub state_id: i32,
    pub name: String,
    pub description: Option<String>,
    pub completed_at: Option<DateTime<Utc>>,
}