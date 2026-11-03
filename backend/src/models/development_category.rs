use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DevelopmentCategory {
    pub item_id: i32,
    pub area_id: i32,
    pub name: String,
}
