use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct Document {
    pub domain_node_id: i32,
    pub content: String,
    pub completeness_level_id: Option<i32>,
    pub flashcard_status_id: Option<i32>,
    pub verification_status_id: Option<i32>,
    pub verification_level_id: Option<i32>,
}
