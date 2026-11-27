use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use serde_json::Value;
use uuid::Uuid;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Source {
    pub id: Uuid,
    pub resource_type_id: i16,
    pub resource_type: String,
    pub title: Option<String>,
    pub url: String,
    pub description: Option<String>,
    pub thumbnail_url: Option<String>,
    pub favicon: Option<Vec<u8>>,
    pub favicon_mime_type: Option<String>,
    pub metadata: Value,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
}

#[derive(Debug, Deserialize)]
pub struct CreateSourceRequest {
    pub url: String,
}

#[derive(Debug, Serialize)]
pub struct CreateSourceResponse {
    pub id: Uuid,
}

#[derive(Debug, Deserialize)]
pub struct UpdateSourceTitleRequest {
    pub title: String,
}

pub struct Favicon {
    pub data: Vec<u8>,
    pub mime_type: String,
}
