use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DevelopmentArea {
    pub id: i32,
    pub name: String,
    pub description: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct CreateDevelopmentArea {
    pub name: String,
    pub description: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateDevelopmentArea {
    pub name: Option<String>,
    pub description: Option<String>,
}
