use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use serde_json::Value;

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
pub struct Progression {
    pub id: i32,
    pub name: String,
    pub personal_relevance_id: Option<i32>,
    pub lasting_impact_id: Option<i32>,
    pub motivation: Option<String>,
    pub desired_outcome: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow)]
pub struct ProgressionStage {
    pub id: i32,
    pub progression_id: i32,
    pub stage: i32,
    pub name: Option<String>,
    pub description: Option<String>,
    pub is_completed: bool,
    pub completed_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct ProgressionStageDetails {
    pub id: i32,
    pub stage: i32,
    pub name: Option<String>,
    pub pursuits: Vec<ProgressionPursuit>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct ProgressionPursuit {
    pub id: i32,
    pub name: String,
    pub progression_stage_id: Option<i32>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct ProgressionDetails {
    pub id: i32,
    pub name: String,
    pub personal_relevance_id: Option<i32>,
    pub lasting_impact_id: Option<i32>,
    pub stages: Vec<ProgressionStageDetails>,
}

#[derive(Debug, Serialize)]
pub struct ProgressionOverview {
    pub progressions: Vec<ProgressionDetails>,
    pub pursuits: Vec<ProgressionPursuit>,
}

#[derive(Debug, Deserialize)]
pub struct CreateProgressionRequest {
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct ProgressionFieldUpdateRequest {
    pub value: Value,
}

#[repr(i32)]
#[derive(Clone, Copy)]
pub enum ProgressionField {
    Name = 1,
    PersonalRelevanceId = 2,
    LastingImpactId = 3,
}

impl TryFrom<i32> for ProgressionField {
    type Error = ();

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(Self::Name),
            2 => Ok(Self::PersonalRelevanceId),
            3 => Ok(Self::LastingImpactId),
            _ => Err(()),
        }
    }
}

pub enum ProgressionFieldUpdate {
    Name(String),
    PersonalRelevanceId(Option<i32>),
    LastingImpactId(Option<i32>),
}

#[derive(Debug, Deserialize)]
pub struct CreateProgressionStageRequest {
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct AssignPursuitRequest {
    pub pursuit_id: i32,
}
