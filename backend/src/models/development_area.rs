use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DevelopmentArea {
    pub id: i32,
    pub name: String,
    pub description: Option<String>,
}
