use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::models::comment::Comment;

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct Skill {
    pub id: i32,
    pub domain_node_id: i32,
    pub parent_skill_id: Option<i32>,
    pub name: String,
    pub psychomotor_level_id: Option<i32>,
    #[sqlx(skip)]
    pub comments: Vec<Comment>,
}

#[derive(Debug, Deserialize)]
pub struct CreateSkillRequest {
    pub parent_skill_id: Option<i32>,
    pub name: String,
    pub psychomotor_level_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct SkillFieldUpdateRequest {
    pub value: Value,
}

#[repr(i32)]
#[derive(Clone, Copy)]
pub enum SkillField {
    Name = 1,
    PsychomotorLevelId = 2,
}

impl TryFrom<i32> for SkillField {
    type Error = ();

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(Self::Name),
            2 => Ok(Self::PsychomotorLevelId),
            _ => Err(()),
        }
    }
}

pub enum SkillFieldUpdate {
    Name(String),
    PsychomotorLevelId(Option<i32>),
}
