use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::models::comment::Comment;

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct Domain {
    pub id: i32,
    pub name: String,
    pub color: Option<String>,
    pub icon: Option<Vec<u8>>,
}

#[derive(Debug, Deserialize)]
pub struct DomainUpdateRequest {
    pub color: Option<String>,
    pub icon: Option<Vec<u8>>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct DomainNode {
    pub id: i32,
    pub domain_id: i32,
    pub parent_id: Option<i32>,
    pub knowledge_level_id: Option<i32>,
    pub name: String,
    pub is_system_value: bool,
    pub last_reviewed_at: DateTime<Utc>,
    pub has_document: bool,
    pub flashcard_count: i64,
    pub attachment_count: i64,
    pub pursuit_count: i64,
    pub task_count: i64,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: Option<i32>,
    pub verification_status_id: Option<i32>,
    pub verification_level_id: Option<i32>,
    #[sqlx(skip)]
    pub comments: Vec<Comment>,
}

#[derive(Debug, Deserialize)]
pub struct DomainNodeNameRequest {
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct DomainNodeFieldUpdateRequest {
    pub value: Value,
}

#[repr(i32)]
#[derive(Clone, Copy)]
pub enum DomainNodeField {
    Name = 1,
    KnowledgeLevelId = 2,
    CompletenessLevelId = 3,
    FlashcardStatusId = 4,
    VerificationStatusId = 5,
    VerificationLevelId = 6,
}

impl TryFrom<i32> for DomainNodeField {
    type Error = ();

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(Self::Name),
            2 => Ok(Self::KnowledgeLevelId),
            3 => Ok(Self::CompletenessLevelId),
            4 => Ok(Self::FlashcardStatusId),
            5 => Ok(Self::VerificationStatusId),
            6 => Ok(Self::VerificationLevelId),
            _ => Err(()),
        }
    }
}

pub enum DomainNodeFieldUpdate {
    Name(String),
    KnowledgeLevelId(Option<i32>),
    CompletenessLevelId(Option<i32>),
    FlashcardStatusId(Option<i32>),
    VerificationStatusId(Option<i32>),
    VerificationLevelId(Option<i32>),
}

#[derive(Debug, Serialize)]
pub struct DomainTree {
    pub domains: Vec<Domain>,
    pub nodes: Vec<DomainNode>,
}
