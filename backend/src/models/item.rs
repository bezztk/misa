use sqlx::types::chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow)]
pub struct Item {
    pub id: i64,
    pub r#type: i32,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub is_archived: bool,
    pub version: i32,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct ItemHierarchy {
    pub child_id: i32,
    pub parent_id: i32,
}

#[derive(Debug, Serialize)]
pub struct AllowedTypeRelation {
    pub parent_type: &'static str,
    pub child_type: &'static str,
}

#[derive(Debug, Deserialize)]
pub struct UpdateItemParent {
    pub parent_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateItemArchive {
    pub is_archived: bool,
}
