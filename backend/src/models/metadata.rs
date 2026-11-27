use serde::{Deserialize, Serialize};

use crate::models::flashcard::{FlashcardRetentionClass, FlashcardReviewState};
use crate::models::idea::{IdeaPhase, IdeaState};
use crate::models::pursuit::PursuitRequirementType;
use crate::models::routine::SystemTaskRoutineExecutionState;
use crate::models::session::{SystemTaskSessionPreset, SystemTaskSessionState};
use crate::models::system_task::{SystemTaskPriority, SystemTaskState};

macro_rules! lookup_model {
    ($name:ident) => {
        #[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
        pub struct $name {
            pub id: i32,
            pub name: String,
            pub display_name: String,
        }
    };
}

lookup_model!(PersonalRelevanceLevel);
lookup_model!(LastingImpact);
lookup_model!(DocumentCompletenessLevel);
lookup_model!(DocumentFlashcardStatus);
lookup_model!(DocumentVerificationStatus);
lookup_model!(DocumentVerificationLevel);
lookup_model!(MediaRatingLookup);

#[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
pub struct KnowledgeLevel {
    pub id: i32,
    pub name: String,
    pub display_name: String,
    pub description: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Metadata {
    pub personal_relevance_levels: Vec<PersonalRelevanceLevel>,
    pub lasting_impacts: Vec<LastingImpact>,
    pub knowledge_levels: Vec<KnowledgeLevel>,
    pub document_completeness_levels: Vec<DocumentCompletenessLevel>,
    pub document_flashcard_statuses: Vec<DocumentFlashcardStatus>,
    pub document_verification_statuses: Vec<DocumentVerificationStatus>,
    pub document_verification_levels: Vec<DocumentVerificationLevel>,
    pub flashcard_retention_classes: Vec<FlashcardRetentionClass>,
    pub flashcard_review_states: Vec<FlashcardReviewState>,
    pub idea_phases: Vec<IdeaPhase>,
    pub idea_states: Vec<IdeaState>,
    pub media_ratings: Vec<MediaRatingLookup>,
    pub pursuit_requirement_types: Vec<PursuitRequirementType>,
    pub systemtask_states: Vec<SystemTaskState>,
    pub systemtask_priorities: Vec<SystemTaskPriority>,
    pub systemtask_routine_execution_states: Vec<SystemTaskRoutineExecutionState>,
    pub systemtask_session_presets: Vec<SystemTaskSessionPreset>,
    pub systemtask_session_states: Vec<SystemTaskSessionState>,
}
