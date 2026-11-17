use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct Skill {
    pub id: i32,
    pub domain_node_id: i32,
    pub parent_skill_id: Option<i32>,
    pub name: String,
    pub psychomotor_level_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct CreateSkillRequest {
    pub parent_skill_id: Option<i32>,
    pub name: String,
    pub psychomotor_level_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateSkillRequest {
    pub name: String,
    pub psychomotor_level_id: Option<i32>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
#[expect(
    dead_code,
    reason = "database entity retained for deferred evaluation functionality"
)]
pub struct SkillEvaluation {
    pub id: i32,
    pub skill_id: i32,
    pub psychomotor_level_id: i32,
    pub evaluated_at: DateTime<Utc>,
    pub note: Option<String>,
}
