use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Skill {
    pub item_id: i32,
    pub area_id: i32,
    pub state_id: i32,
    pub name: String,
    pub description: Option<String>,
}
