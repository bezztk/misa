use crate::models::flashcard::{FlashcardQueueContext, FlashcardReviewContext};
use crate::services::flashcard_review_service;
use chrono::Utc;
use sqlx::PgPool;

#[derive(Debug)]
pub enum ReviewFlashcardError {
    NotFound,
    Database(sqlx::Error),
    Calculation(fsrs::FSRSError),
}

impl From<sqlx::Error> for ReviewFlashcardError {
    fn from(error: sqlx::Error) -> Self {
        Self::Database(error)
    }
}

impl From<fsrs::FSRSError> for ReviewFlashcardError {
    fn from(error: fsrs::FSRSError) -> Self {
        Self::Calculation(error)
    }
}

pub async fn create_flashcard(
    pool: &PgPool,
    domain_node_id: i32,
    front: &str,
    back: &str,
    retention_class_id: i32,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO flashcards
            (domain_node_id, front, back, retention_class_id)
         VALUES ($1, $2, $3, $4)
         RETURNING id",
    )
    .bind(domain_node_id)
    .bind(front)
    .bind(back)
    .bind(retention_class_id)
    .fetch_one(pool)
    .await
}

pub async fn get_due_flashcard_contexts(
    pool: &PgPool,
) -> Result<Vec<FlashcardQueueContext>, sqlx::Error> {
    sqlx::query_as::<_, FlashcardQueueContext>(
        "SELECT
            f.id,
            f.front,
            f.back,
            f.review_state_id,
            f.stability,
            f.difficulty,
            f.last_reviewed_at,
            r.desired_retention::REAL AS desired_retention,
            (
                SELECT fr.rating
                FROM flashcard_reviews fr
                WHERE fr.flashcard_id = f.id
                ORDER BY fr.reviewed_at DESC, fr.id DESC
                LIMIT 1
            ) AS last_rating
         FROM flashcards f
         JOIN flashcard_retention_classes r ON r.id = f.retention_class_id
         WHERE f.due_at IS NULL OR f.due_at <= NOW()
         ORDER BY f.due_at NULLS FIRST, f.id",
    )
    .fetch_all(pool)
    .await
}

pub async fn delete_flashcard(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM flashcards WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}

pub async fn review_flashcard(
    pool: &PgPool,
    id: i32,
    rating: i16,
    duration_ms: i32,
) -> Result<(), ReviewFlashcardError> {
    let mut tx = pool.begin().await?;
    let context = sqlx::query_as::<_, FlashcardReviewContext>(
        "SELECT
            f.review_state_id,
            f.stability,
            f.difficulty,
            f.last_reviewed_at,
            r.desired_retention::REAL AS desired_retention
         FROM flashcards f
         JOIN flashcard_retention_classes r ON r.id = f.retention_class_id
         WHERE f.id = $1
         FOR UPDATE OF f",
    )
    .bind(id)
    .fetch_optional(&mut *tx)
    .await?
    .ok_or(ReviewFlashcardError::NotFound)?;

    let last_rating = sqlx::query_scalar::<_, i16>(
        "SELECT rating
         FROM flashcard_reviews
         WHERE flashcard_id = $1
         ORDER BY reviewed_at DESC, id DESC
         LIMIT 1",
    )
    .bind(id)
    .fetch_optional(&mut *tx)
    .await?;

    let schedule = flashcard_review_service::calculate_review_schedule(
        context.review_state_id,
        last_rating,
        context.stability,
        context.difficulty,
        context.last_reviewed_at,
        context.desired_retention,
        rating,
        Utc::now(),
    )?;

    sqlx::query(
        "UPDATE flashcards
         SET review_state_id = $1,
             stability = $2,
             difficulty = $3,
             last_reviewed_at = $4,
             due_at = $5
         WHERE id = $6",
    )
    .bind(schedule.review_state_id)
    .bind(schedule.stability)
    .bind(schedule.difficulty)
    .bind(schedule.reviewed_at)
    .bind(schedule.next_due_at)
    .bind(id)
    .execute(&mut *tx)
    .await?;

    sqlx::query(
        "INSERT INTO flashcard_reviews
            (flashcard_id, rating, stability, difficulty,
             review_state_before_id, review_state_after_id,
             reviewed_at, duration_ms, next_due_at)
         VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)",
    )
    .bind(id)
    .bind(rating)
    .bind(schedule.stability)
    .bind(schedule.difficulty)
    .bind(context.review_state_id)
    .bind(schedule.review_state_id)
    .bind(schedule.reviewed_at)
    .bind(duration_ms)
    .bind(schedule.next_due_at)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;
    Ok(())
}
