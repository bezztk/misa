use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::models::comment::Comment;

#[derive(Debug, Clone, PartialEq, sqlx::FromRow, Serialize, Deserialize)]
pub struct FlashcardRetentionClass {
    pub id: i32,
    pub name: String,
    pub display_name: String,
    pub desired_retention: f32,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct FlashcardReviewState {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FlashcardReviewStateId {
    New = 1,
    Learning = 2,
    Review = 3,
    Relearning = 4,
}

#[derive(Debug, Clone, PartialEq, sqlx::FromRow)]
#[expect(
    dead_code,
    reason = "database entity retained alongside specialized API projections"
)]
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

#[derive(Debug, Clone, PartialEq, sqlx::FromRow)]
#[expect(
    dead_code,
    reason = "database entity retained alongside specialized review projections"
)]
pub struct FlashcardReview {
    pub id: i32,
    pub flashcard_id: i32,
    pub rating: i16,
    pub stability: f32,
    pub difficulty: f32,
    pub review_state_before_id: i32,
    pub review_state_after_id: i32,
    pub reviewed_at: DateTime<Utc>,
    pub duration_ms: i32,
    pub next_due_at: DateTime<Utc>,
}

#[derive(Debug, Deserialize)]
pub struct CreateFlashcardRequest {
    pub front: String,
    pub back: String,
}

#[derive(Debug, Deserialize)]
pub struct FlashcardFieldUpdateRequest {
    pub value: Value,
}

#[repr(i32)]
#[derive(Clone, Copy)]
pub enum FlashcardField {
    Front = 1,
    Back = 2,
}

impl TryFrom<i32> for FlashcardField {
    type Error = ();

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(Self::Front),
            2 => Ok(Self::Back),
            _ => Err(()),
        }
    }
}

pub enum FlashcardFieldUpdate {
    Front(String),
    Back(String),
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct EditableFlashcard {
    pub id: i32,
    pub front: String,
    pub back: String,
    #[sqlx(skip)]
    pub comments: Vec<Comment>,
}

#[derive(Debug, Deserialize)]
pub struct ReviewFlashcardRequest {
    pub rating: i16,
    pub duration_ms: i32,
}

#[derive(Debug, Clone, PartialEq, sqlx::FromRow)]
pub struct FlashcardReviewContext {
    pub review_state_id: i32,
    pub stability: Option<f32>,
    pub difficulty: Option<f32>,
    pub last_reviewed_at: Option<DateTime<Utc>>,
    pub desired_retention: f32,
    pub last_rating: Option<i16>,
}

#[derive(Debug, Clone, PartialEq, sqlx::FromRow)]
pub struct FlashcardQueueContext {
    pub id: i32,
    pub front: String,
    pub back: String,
    pub review_state_id: i32,
    pub retention_class_id: i32,
    pub stability: Option<f32>,
    pub difficulty: Option<f32>,
    pub last_reviewed_at: Option<DateTime<Utc>>,
    pub desired_retention: f32,
    pub last_rating: Option<i16>,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
pub struct FlashcardQueueItem {
    pub id: i32,
    pub front: String,
    pub back: String,
    pub review_state_id: i32,
    pub retention_class_id: i32,
    pub learning_step: i16,
    pub stability: Option<f32>,
    pub difficulty: Option<f32>,
    pub preview: FlashcardReviewPreview,
    pub comments: Vec<Comment>,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
pub struct FlashcardReviewPreview {
    pub again: f32,
    pub hard: f32,
    pub good: f32,
    pub easy: f32,
}
