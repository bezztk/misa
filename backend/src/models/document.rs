use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct Document {
    pub domain_node_id: i32,
    pub content: String,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: Option<i32>,
    pub verification_status_id: Option<i32>,
    pub verification_level_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct DocumentContentRequest {
    pub content: String,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct DocumentVersion {
    pub version: i32,
    pub content: String,
    pub captured_at: DateTime<Utc>,
}
