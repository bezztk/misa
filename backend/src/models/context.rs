use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Context {
    pub id: i32,
    pub title: String,
    pub icon: Option<Vec<u8>>,
    pub color: Option<String>,
    pub entity_count: i64,
}

#[derive(Debug, Deserialize)]
pub struct SaveContextRequest {
    pub title: String,
    pub icon: Option<Vec<u8>>,
    pub color: Option<String>,
}
