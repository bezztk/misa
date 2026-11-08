use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Topic {
    pub entity_id: i32,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub version: i32,
    pub name: String,
    pub understanding_level_id: i32,
}

#[derive(Debug, Deserialize)]
pub struct CreateTopic {
    pub name: String,
    pub understanding_level_id: i32,
}

#[derive(Debug, Deserialize)]
pub struct UpdateTopic {
    pub name: String,
    pub understanding_level_id: i32,
}
