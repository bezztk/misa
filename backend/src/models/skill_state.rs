use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct SkillStateRow {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SkillState {
    Planned = 1,
    Developing = 2,
    Maintaining = 3,
    Paused = 4,
    Inactive = 5,
}
