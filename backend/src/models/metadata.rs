use crate::models::document_metadata::{
    DocumentCompletenessLevel, DocumentFlashcardStatus, DocumentVerificationLevel,
    DocumentVerificationStatus,
};
use crate::models::task_metadata::{TaskGroup, TaskPriority, TaskStateRow};
use crate::models::topic_metadata::TopicUnderstandingLevel;
use serde::Serialize;

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
