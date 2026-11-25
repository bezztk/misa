use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

use crate::models::comment::Comment;

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct PursuitRequirementType {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PursuitRequirementTypeId {
    KnowledgeLevel = 1,
    CompletenessLevel = 3,
    FlashcardStatus = 4,
    VerificationStatus = 5,
    VerificationLevel = 6,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
pub struct Pursuit {
    pub id: i32,
    pub domain_node_id: i32,
    pub progression_stage_id: Option<i32>,
    pub name: String,
    pub achieved_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct PursuitRequirement {
    pub id: i32,
    pub pursuit_id: i32,
    pub requirement_type_id: i32,
    pub knowledge_level_id: Option<i32>,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: Option<i32>,
    pub verification_status_id: Option<i32>,
    pub verification_level_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct CreatePursuitRequest {
    pub name: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct PursuitDetails {
    pub id: i32,
    pub domain_node_id: i32,
    pub name: String,
    pub requirements: Vec<PursuitRequirement>,
    pub comments: Vec<Comment>,
}

#[derive(Debug, Deserialize)]
pub struct UpdatePursuitRequirementsRequest {
    pub knowledge_level_id: Option<i32>,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: Option<i32>,
    pub verification_status_id: Option<i32>,
    pub verification_level_id: Option<i32>,
}
