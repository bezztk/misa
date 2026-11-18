use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct Attachment {
    pub id: i32,
    pub domain_node_id: i32,
    pub original_name: String,
    pub storage_name: String,
    pub size_bytes: i64,
    pub created_at: DateTime<Utc>,
}

#[derive(Debug, Deserialize)]
pub struct CreateAttachmentRequest {
    pub original_name: String,
    pub storage_name: String,
    pub size_bytes: i64,
}
