use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct TaskGroup {
    pub id: i32,
    pub name: String,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct TaskStateRow {
    pub id: i32,
    pub name: String,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct TaskPriority {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TaskState {
    Open = 1,
    InProgress = 2,
    Completed = 3,
    Cancelled = 4,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct TopicUnderstandingLevel {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentCompletenessLevel {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentFlashcardStatus {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentVerificationStatus {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentVerificationLevel {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, Serialize)]
pub struct Metadata {
    pub task_groups: Vec<TaskGroup>,
    pub task_states: Vec<TaskStateRow>,
    pub task_priorities: Vec<TaskPriority>,
    pub topic_understanding_levels: Vec<TopicUnderstandingLevel>,
    pub document_completeness_levels: Vec<DocumentCompletenessLevel>,
    pub document_flashcard_statuses: Vec<DocumentFlashcardStatus>,
    pub document_verification_statuses: Vec<DocumentVerificationStatus>,
    pub document_verification_levels: Vec<DocumentVerificationLevel>,
}
