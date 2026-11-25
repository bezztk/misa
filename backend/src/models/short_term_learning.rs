use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use sqlx::FromRow;

pub const NEW: i32 = 1;
pub const LEARNING: i32 = 2;
pub const COMPLETED: i32 = 3;

#[derive(FromRow, Serialize)]
pub struct ShortTermLearningSet {
    pub id: i32,
    pub title: String,
    pub bidirectional: bool,
    pub card_count: i64,
    pub new_count: i64,
    pub learning_count: i64,
    pub completed_count: i64,
}

#[derive(Deserialize)]
pub struct CreateShortTermLearningSet {
    pub title: String,
    pub bidirectional: bool,
}

#[derive(FromRow, Serialize)]
pub struct ShortTermLearningCard {
    pub id: i32,
    pub front: String,
    pub hint: Option<String>,
    pub back: String,
    pub front_to_back_state_id: i32,
    pub back_to_front_state_id: Option<i32>,
    pub front_to_back_last_rating_id: Option<i32>,
    pub front_to_back_due_at: Option<DateTime<Utc>>,
    pub back_to_front_last_rating_id: Option<i32>,
    pub back_to_front_due_at: Option<DateTime<Utc>>,
}

#[derive(FromRow, Serialize)]
pub struct ShortTermLearningCardRating {
    pub id: i32,
    pub display_name: String,
    pub interval_minutes: Option<i32>,
}

#[derive(Serialize)]
pub struct ShortTermLearningCards {
    pub cards: Vec<ShortTermLearningCard>,
    pub ratings: Vec<ShortTermLearningCardRating>,
}

#[derive(Deserialize, Serialize)]
pub struct ShortTermLearningCardContent {
    pub front: String,
    pub hint: Option<String>,
    pub back: String,
}

impl ShortTermLearningCardContent {
    pub fn is_valid(&self) -> bool {
        !self.front.trim().is_empty() && !self.back.trim().is_empty()
    }
}

#[derive(Deserialize)]
pub struct RateShortTermLearningCard {
    pub reverse: bool,
    pub rating_id: i32,
}

#[derive(FromRow, Serialize)]
pub struct ShortTermLearningProgress {
    pub state_id: i32,
    pub last_rating_id: i32,
    pub due_at: Option<DateTime<Utc>>,
}
