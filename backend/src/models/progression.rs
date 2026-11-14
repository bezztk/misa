use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct Progression {
    pub id: i32,
    pub name: String,
    pub personal_relevance_id: Option<i32>,
    pub lasting_impact_id: Option<i32>,
    pub motivation: Option<String>,
    pub desired_outcome: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct ProgressionStage {
    pub id: i32,
    pub progression_id: i32,
    pub stage: i32,
    pub name: Option<String>,
    pub description: Option<String>,
    pub is_completed: bool,
    pub completed_at: Option<DateTime<Utc>>,
}
