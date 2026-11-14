use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, sqlx::FromRow, Serialize, Deserialize)]
pub struct FlashcardRetentionClass {
    pub id: i32,
    pub name: String,
    pub display_name: String,
    pub desired_retention: f32,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum FlashcardRetentionClassId {
    Supplementary = 1,
    General = 2,
    Specialized = 3,
    Fundamental = 4,
    Essential = 5,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct FlashcardReviewState {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum FlashcardReviewStateId {
    New = 1,
    Learning = 2,
    Review = 3,
    Relearning = 4,
}

#[derive(Debug, Clone, PartialEq, sqlx::FromRow, Serialize, Deserialize)]
pub struct Flashcard {
    pub id: i32,
    pub domain_node_id: i32,
    pub front: String,
    pub back: String,
    pub hint: Option<String>,
    pub explanation: Option<String>,
    pub review_state_id: i32,
    pub retention_class_id: i32,
    pub stability: Option<f32>,
    pub difficulty: Option<f32>,
    pub last_reviewed_at: Option<DateTime<Utc>>,
    pub due_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Clone, PartialEq, sqlx::FromRow, Serialize, Deserialize)]
pub struct FlashcardReview {
    pub id: i32,
    pub flashcard_id: i32,
    pub rating: i16,
    pub stability: f32,
    pub difficulty: f32,
    pub review_state_before_id: i32,
    pub review_state_after_id: i32,
    pub reviewed_at: DateTime<Utc>,
    pub duration_ms: Option<i32>,
    pub next_due_at: DateTime<Utc>,
}
