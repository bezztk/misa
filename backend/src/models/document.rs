use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Document {
    pub entity_id: i32,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub version: i32,
    pub topic_id: i32,
    pub content: String,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: i32,
    pub verification_status_id: i32,
    pub verification_level_id: Option<i32>,
    pub last_verified_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateDocument {
    pub content: String,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: i32,
    pub verification_status_id: i32,
    pub verification_level_id: Option<i32>,
    pub last_verified_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateDocumentContent {
    pub content: String,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentRevision {
    pub id: i32,
    pub document_id: i32,
    pub content: String,
    pub created_at: DateTime<Utc>,
}

#[derive(Debug, Deserialize)]
pub struct CreateDocumentRevision {
    pub content: String,
}
