use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct PursuitRequirementType {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum PursuitRequirementTypeId {
    CognitiveLevel = 1,
    PsychomotorLevel = 2,
    CompletenessLevel = 3,
    FlashcardStatus = 4,
    VerificationStatus = 5,
    VerificationLevel = 6,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct Pursuit {
    pub id: i32,
    pub domain_node_id: i32,
    pub progression_stage_id: Option<i32>,
    pub name: String,
    pub achieved_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct PursuitRequirement {
    pub id: i32,
    pub pursuit_id: i32,
    pub requirement_type_id: i32,
    pub skill_id: Option<i32>,
    pub cognitive_level_id: Option<i32>,
    pub psychomotor_level_id: Option<i32>,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: Option<i32>,
    pub verification_status_id: Option<i32>,
    pub verification_level_id: Option<i32>,
}
