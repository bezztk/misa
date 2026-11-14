use crate::models::domain::Domain;
use crate::models::flashcard::{FlashcardRetentionClass, FlashcardReviewState};
use crate::models::metadata::{
    CognitiveLevel, DocumentCompletenessLevel, DocumentFlashcardStatus, DocumentVerificationLevel,
    DocumentVerificationStatus, LastingImpact, Metadata, PersonalRelevanceLevel, PsychomotorLevel,
};
use crate::models::pursuit::PursuitRequirementType;
use crate::models::routine::SystemTaskRoutineExecutionState;
use crate::models::session::{SystemTaskSessionPreset, SystemTaskSessionState};
use crate::models::system_task::{SystemTaskGroup, SystemTaskPriority, SystemTaskState};
use sqlx::PgPool;

pub async fn get_metadata(pool: &PgPool) -> Result<Metadata, sqlx::Error> {
    Ok(Metadata {
        personal_relevance_levels: sqlx::query_as::<_, PersonalRelevanceLevel>(
            "SELECT id, name, display_name FROM personal_relevance_levels ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        lasting_impacts: sqlx::query_as::<_, LastingImpact>(
            "SELECT id, name, display_name FROM lasting_impacts ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        cognitive_levels: sqlx::query_as::<_, CognitiveLevel>(
            "SELECT id, name, display_name FROM cognitive_levels ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        psychomotor_levels: sqlx::query_as::<_, PsychomotorLevel>(
            "SELECT id, name, display_name FROM psychomotor_levels ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        document_completeness_levels: sqlx::query_as::<_, DocumentCompletenessLevel>(
            "SELECT id, name, display_name FROM document_completeness_levels ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        document_flashcard_statuses: sqlx::query_as::<_, DocumentFlashcardStatus>(
            "SELECT id, name, display_name FROM document_flashcard_statuses ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        document_verification_statuses: sqlx::query_as::<_, DocumentVerificationStatus>(
            "SELECT id, name, display_name FROM document_verification_statuses ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        document_verification_levels: sqlx::query_as::<_, DocumentVerificationLevel>(
            "SELECT id, name, display_name FROM document_verification_levels ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        domains: sqlx::query_as::<_, Domain>("SELECT id, name FROM domains ORDER BY id")
            .fetch_all(pool)
            .await?,
        flashcard_retention_classes: sqlx::query_as::<_, FlashcardRetentionClass>(
            "SELECT id, name, display_name, desired_retention::REAL AS desired_retention
             FROM flashcard_retention_classes
             ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        flashcard_review_states: sqlx::query_as::<_, FlashcardReviewState>(
            "SELECT id, name FROM flashcard_review_states ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        pursuit_requirement_types: sqlx::query_as::<_, PursuitRequirementType>(
            "SELECT id, name FROM pursuit_requirement_types ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        systemtask_groups: sqlx::query_as::<_, SystemTaskGroup>(
            "SELECT id, name FROM systemtask_groups ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        systemtask_states: sqlx::query_as::<_, SystemTaskState>(
            "SELECT id, name, display_name FROM systemtask_states ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        systemtask_priorities: sqlx::query_as::<_, SystemTaskPriority>(
            "SELECT id, name, display_name FROM systemtask_priorities ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        systemtask_routine_execution_states: sqlx::query_as::<_, SystemTaskRoutineExecutionState>(
            "SELECT id, name, display_name
             FROM systemtask_routine_execution_states
             ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        systemtask_session_presets: sqlx::query_as::<_, SystemTaskSessionPreset>(
            "SELECT id, name, planned_duration_minutes
             FROM systemtask_session_presets
             ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
        systemtask_session_states: sqlx::query_as::<_, SystemTaskSessionState>(
            "SELECT id, name, display_name FROM systemtask_session_states ORDER BY id",
        )
        .fetch_all(pool)
        .await?,
    })
}
