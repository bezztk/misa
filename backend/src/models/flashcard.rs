use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Flashcard {
    pub id: i32,
    pub document_id: i32,
    pub front: String,
    pub back: String,
    pub hint: Option<String>,
    pub explanation: Option<String>,
    pub state: i16,
    pub retention_levels_id: i32,
    pub stability: Option<f32>,
    pub difficulty: Option<f32>,
    pub last_reviewed_at: Option<DateTime<Utc>>,
    pub due_at: Option<DateTime<Utc>>,
    pub is_enabled: bool,
}

#[derive(Debug, Deserialize)]
pub struct CreateFlashcard {
    pub document_id: i32,
    pub front: String,
    pub back: String,
    pub hint: Option<String>,
    pub explanation: Option<String>,
    pub retention_levels_id: i32,
    pub is_enabled: bool,
}

#[derive(Debug, Deserialize)]
pub struct UpdateFlashcard {
    pub front: String,
    pub back: String,
    pub hint: Option<String>,
    pub explanation: Option<String>,
    pub retention_levels_id: i32,
    pub is_enabled: bool,
}

#[derive(Debug, Deserialize)]
pub struct CreateFlashcardReview {
    pub rating: i16,
    pub duration_ms: i32,
}

#[derive(Debug, sqlx::FromRow)]
pub struct FlashcardReviewContext {
    pub stability: Option<f32>,
    pub difficulty: Option<f32>,
    pub last_reviewed_at: Option<DateTime<Utc>>,
    pub desired_retention: f32,
}

#[derive(Debug, Serialize)]
pub struct FlashcardReviewPreview {
    pub again: i32,
    pub hard: i32,
    pub good: i32,
    pub easy: i32,
}
