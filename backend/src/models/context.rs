use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Context {
    pub id: i32,
    pub title: String,
}

#[derive(Debug, Deserialize)]
pub struct ContextTitleRequest {
    pub title: String,
}
