use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Skill {
    pub item_id: i32,
    pub area_id: i32,
    pub state_id: i32,
    pub name: String,
    pub description: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct CreateSkill {
    pub area_id: i32,
    pub state_id: Option<i32>,
    pub name: String,
    pub description: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateSkill {
    pub state_id: Option<i32>,
    pub name: Option<String>,
    pub description: Option<String>,
}
