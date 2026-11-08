use crate::models::metadata::{
    DocumentCompletenessLevel, DocumentFlashcardStatus, DocumentVerificationLevel,
    DocumentVerificationStatus, Metadata, TaskGroup, TaskPriority, TaskStateRow,
    TopicUnderstandingLevel,
};
use sqlx::PgPool;

pub async fn get_task_groups(pool: &PgPool) -> Result<Vec<TaskGroup>, sqlx::Error> {
    sqlx::query_as("SELECT id, name FROM task_groups ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_task_states(pool: &PgPool) -> Result<Vec<TaskStateRow>, sqlx::Error> {
    sqlx::query_as("SELECT id, name, display_name FROM task_states ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_task_priorities(pool: &PgPool) -> Result<Vec<TaskPriority>, sqlx::Error> {
    sqlx::query_as("SELECT id, name, display_name FROM task_priorities ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_topic_understanding_levels(
    pool: &PgPool,
) -> Result<Vec<TopicUnderstandingLevel>, sqlx::Error> {
    sqlx::query_as(
        "SELECT id, name, display_name
         FROM topic_understanding_levels
         ORDER BY id",
    )
    .fetch_all(pool)
    .await
}

pub async fn get_document_completeness_levels(
    pool: &PgPool,
) -> Result<Vec<DocumentCompletenessLevel>, sqlx::Error> {
    sqlx::query_as("SELECT id, name, display_name FROM document_completeness_levels ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_document_flashcard_statuses(
    pool: &PgPool,
) -> Result<Vec<DocumentFlashcardStatus>, sqlx::Error> {
    sqlx::query_as("SELECT id, name, display_name FROM document_flashcard_statuses ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_document_verification_statuses(
    pool: &PgPool,
) -> Result<Vec<DocumentVerificationStatus>, sqlx::Error> {
    sqlx::query_as("SELECT id, name, display_name FROM document_verification_statuses ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_document_verification_levels(
    pool: &PgPool,
) -> Result<Vec<DocumentVerificationLevel>, sqlx::Error> {
    sqlx::query_as("SELECT id, name, display_name FROM document_verification_levels ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_metadata(pool: &PgPool) -> Result<Metadata, sqlx::Error> {
    Ok(Metadata {
        task_groups: get_task_groups(pool).await?,
        task_states: get_task_states(pool).await?,
        task_priorities: get_task_priorities(pool).await?,
        topic_understanding_levels: get_topic_understanding_levels(pool).await?,
        document_completeness_levels: get_document_completeness_levels(pool).await?,
        document_flashcard_statuses: get_document_flashcard_statuses(pool).await?,
        document_verification_statuses: get_document_verification_statuses(pool).await?,
        document_verification_levels: get_document_verification_levels(pool).await?,
    })
}
