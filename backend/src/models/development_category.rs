use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DevelopmentCategory {
    pub item_id: i32,
    pub area_id: i32,
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct CreateDevelopmentCategory {
    pub area_id: i32,
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct UpdateDevelopmentCategory {
    pub name: Option<String>,
}
