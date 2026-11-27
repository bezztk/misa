use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::models::comment::Comment;

pub const BOOKMARKED_IDEA_PHASE_ID: i32 = 1;
pub const ACTIVE_IDEA_STATE_ID: i32 = 1;

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct IdeaPhase {
    pub id: i32,
    pub name: String,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct IdeaState {
    pub id: i32,
    pub name: String,
    pub display_name: String,
    pub description: String,
}

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize)]
pub struct Idea {
    pub id: i32,
    pub title: String,
    pub content: String,
    pub phase_id: i32,
    pub state_id: i32,
    pub context_id: Option<i32>,
    pub completed_at: Option<DateTime<Utc>>,
    pub created_at: DateTime<Utc>,
    #[sqlx(skip)]
    pub comments: Vec<Comment>,
}

#[derive(Debug, Deserialize)]
pub struct CreateIdeaRequest {
    pub title: String,
    pub content: String,
}

#[derive(Debug, Deserialize)]
pub struct IdeaFieldUpdateRequest {
    pub value: Value,
}

#[derive(Debug, Deserialize)]
pub struct IdeaContextRequest {
    pub context_id: Option<i32>,
}

#[repr(i32)]
#[derive(Clone, Copy)]
pub enum IdeaField {
    Title = 1,
    Content = 2,
    PhaseId = 3,
    StateId = 4,
}

impl TryFrom<i32> for IdeaField {
    type Error = ();

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            1 => Ok(Self::Title),
            2 => Ok(Self::Content),
            3 => Ok(Self::PhaseId),
            4 => Ok(Self::StateId),
            _ => Err(()),
        }
    }
}

pub enum IdeaFieldUpdate {
    Title(String),
    Content(String),
    PhaseId(i32),
    StateId(i32),
}
