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
pub struct UpdateShortTermLearningCard {
    #[serde(flatten)]
    pub content: ShortTermLearningCardContent,
    pub front_to_back_state_id: i32,
    pub back_to_front_state_id: Option<i32>,
}

#[derive(Deserialize)]
pub struct UpdateShortTermLearningState {
    pub reverse: bool,
    pub state_id: i32,
}
