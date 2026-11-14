use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct Domain {
    pub id: i32,
    pub name: String,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct DomainNode {
    pub id: i32,
    pub domain_id: i32,
    pub parent_id: Option<i32>,
    pub cognitive_level_id: Option<i32>,
    pub name: String,
    pub is_system_value: bool,
    pub last_reviewed_at: DateTime<Utc>,
    pub has_document: bool,
    pub flashcard_count: i64,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: Option<i32>,
    pub verification_status_id: Option<i32>,
    pub verification_level_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct DomainNodeNameRequest {
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct DomainNodeUpdateRequest {
    pub name: String,
    pub cognitive_level_id: Option<i32>,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: Option<i32>,
    pub verification_status_id: Option<i32>,
    pub verification_level_id: Option<i32>,
}

#[derive(Debug, Serialize)]
pub struct DomainTree {
    pub domains: Vec<Domain>,
    pub nodes: Vec<DomainNode>,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct CognitiveEvaluation {
    pub id: i32,
    pub domain_node_id: i32,
    pub cognitive_level_id: i32,
    pub evaluated_at: DateTime<Utc>,
    pub note: Option<String>,
}
