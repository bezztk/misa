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
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct CognitiveEvaluation {
    pub id: i32,
    pub domain_node_id: i32,
    pub cognitive_level_id: i32,
    pub evaluated_at: DateTime<Utc>,
    pub note: Option<String>,
}
