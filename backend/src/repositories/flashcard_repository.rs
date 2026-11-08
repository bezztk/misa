use crate::models::flashcard::{Flashcard, FlashcardReviewContext};
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
    document_id: i32,
    front: &str,
    back: &str,
    hint: Option<&str>,
    explanation: Option<&str>,
    retention_levels_id: i32,
    is_enabled: bool,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO flashcards
            (document_id, front, back, hint, explanation, retention_levels_id, is_enabled)
         VALUES ($1, $2, $3, $4, $5, $6, $7)
         RETURNING id",
    )
    .bind(document_id)
    .bind(front)
    .bind(back)
    .bind(hint)
    .bind(explanation)
    .bind(retention_levels_id)
    .bind(is_enabled)
    .fetch_one(pool)
    .await
}

pub async fn get_flashcards(pool: &PgPool) -> Result<Vec<Flashcard>, sqlx::Error> {
    sqlx::query_as::<_, Flashcard>(
        "SELECT
            id,
            document_id,
            front,
            back,
            hint,
            explanation,
            state,
            retention_levels_id,
            stability,
            difficulty,
            last_reviewed_at,
            due_at,
            is_enabled
         FROM flashcards
         ORDER BY id",
    )
    .fetch_all(pool)
    .await
}

pub async fn get_due_flashcards(pool: &PgPool) -> Result<Vec<Flashcard>, sqlx::Error> {
    sqlx::query_as::<_, Flashcard>(
        "SELECT
            id,
            document_id,
            front,
            back,
            hint,
            explanation,
            state,
            retention_levels_id,
            stability,
            difficulty,
            last_reviewed_at,
            due_at,
            is_enabled
         FROM flashcards
         WHERE is_enabled
           AND (last_reviewed_at IS NULL OR due_at <= NOW())
         ORDER BY due_at NULLS FIRST, id",
    )
    .fetch_all(pool)
    .await
}

pub async fn get_flashcard_review_context(
    pool: &PgPool,
    id: i32,
) -> Result<Option<FlashcardReviewContext>, sqlx::Error> {
    sqlx::query_as::<_, FlashcardReviewContext>(
        "SELECT
            f.stability,
            f.difficulty,
            f.last_reviewed_at,
            r.desired_retention::REAL AS desired_retention
         FROM flashcards f
         JOIN flashcard_retention_levels r ON r.id = f.retention_levels_id
         WHERE f.id = $1",
    )
    .bind(id)
    .fetch_optional(pool)
    .await
}

pub async fn update_flashcard(
    pool: &PgPool,
    id: i32,
    front: &str,
    back: &str,
    hint: Option<&str>,
    explanation: Option<&str>,
    retention_levels_id: i32,
    is_enabled: bool,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE flashcards
         SET front = $1,
             back = $2,
             hint = $3,
             explanation = $4,
             retention_levels_id = $5,
             is_enabled = $6
         WHERE id = $7",
    )
    .bind(front)
    .bind(back)
    .bind(hint)
    .bind(explanation)
    .bind(retention_levels_id)
    .bind(is_enabled)
    .bind(id)
    .execute(pool)
    .await?;

    Ok(result.rows_affected() > 0)
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
            f.stability,
            f.difficulty,
            f.last_reviewed_at,
            r.desired_retention::REAL AS desired_retention
         FROM flashcards f
         JOIN flashcard_retention_levels r ON r.id = f.retention_levels_id
         WHERE f.id = $1
         FOR UPDATE OF f",
    )
    .bind(id)
    .fetch_optional(&mut *tx)
    .await?
    .ok_or(ReviewFlashcardError::NotFound)?;

    let schedule = flashcard_review_service::calculate_review_schedule(
        context.stability,
        context.difficulty,
        context.last_reviewed_at,
        context.desired_retention,
        rating,
        Utc::now(),
    )?;

    sqlx::query(
        "UPDATE flashcards
         SET stability = $1,
             difficulty = $2,
             last_reviewed_at = $3,
             due_at = $4
         WHERE id = $5",
    )
    .bind(schedule.stability)
    .bind(schedule.difficulty)
    .bind(schedule.reviewed_at)
    .bind(schedule.due_at)
    .bind(id)
    .execute(&mut *tx)
    .await?;

    sqlx::query(
        "INSERT INTO flashcard_reviews
            (flashcard_id, rating, reviewed_at, duration_ms, scheduled_days,
             stability, difficulty)
         VALUES ($1, $2, $3, $4, $5, $6, $7)",
    )
    .bind(id)
    .bind(rating)
    .bind(schedule.reviewed_at)
    .bind(duration_ms)
    .bind(schedule.scheduled_days)
    .bind(schedule.stability)
    .bind(schedule.difficulty)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;
    Ok(())
}
