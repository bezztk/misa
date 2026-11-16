use crate::models::flashcard::{
    EditableFlashcard, FlashcardQueueContext, FlashcardRetentionClassId, FlashcardReviewContext,
};
use chrono::{DateTime, Utc};
use sqlx::{PgConnection, PgPool};

pub struct NewFlashcardReview {
    pub flashcard_id: i32,
    pub rating: i16,
    pub duration_ms: i32,
    pub review_state_before_id: i32,
    pub review_state_after_id: i32,
    pub stability: f32,
    pub difficulty: f32,
    pub reviewed_at: DateTime<Utc>,
    pub next_due_at: DateTime<Utc>,
}

pub async fn create_flashcard(
    pool: &PgPool,
    domain_node_id: i32,
    front: &str,
    back: &str,
) -> Result<EditableFlashcard, sqlx::Error> {
    sqlx::query_as::<_, EditableFlashcard>(
        "INSERT INTO flashcards
            (domain_node_id, front, back, retention_class_id)
         VALUES ($1, $2, $3, $4)
         RETURNING id, front, back",
    )
    .bind(domain_node_id)
    .bind(front)
    .bind(back)
    .bind(FlashcardRetentionClassId::General as i32)
    .fetch_one(pool)
    .await
}

pub async fn get_flashcards(
    pool: &PgPool,
    domain_node_id: i32,
) -> Result<Vec<EditableFlashcard>, sqlx::Error> {
    sqlx::query_as::<_, EditableFlashcard>(
        "SELECT id, front, back
         FROM flashcards
         WHERE domain_node_id = $1
         ORDER BY id",
    )
    .bind(domain_node_id)
    .fetch_all(pool)
    .await
}

pub async fn update_flashcard(
    pool: &PgPool,
    id: i32,
    front: &str,
    back: &str,
) -> Result<Option<EditableFlashcard>, sqlx::Error> {
    sqlx::query_as::<_, EditableFlashcard>(
        "UPDATE flashcards
         SET front = $2, back = $3
         WHERE id = $1
         RETURNING id, front, back",
    )
    .bind(id)
    .bind(front)
    .bind(back)
    .fetch_optional(pool)
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
            f.retention_class_id,
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

pub async fn get_flashcard_review_context(
    connection: &mut PgConnection,
    id: i32,
) -> Result<Option<FlashcardReviewContext>, sqlx::Error> {
    sqlx::query_as::<_, FlashcardReviewContext>(
        "SELECT
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
         WHERE f.id = $1
         FOR UPDATE OF f",
    )
    .bind(id)
    .fetch_optional(connection)
    .await
}

pub async fn save_flashcard_review(
    connection: &mut PgConnection,
    review: &NewFlashcardReview,
) -> Result<(), sqlx::Error> {
    sqlx::query(
        "UPDATE flashcards
         SET review_state_id = $1,
             stability = $2,
             difficulty = $3,
             last_reviewed_at = $4,
             due_at = $5
         WHERE id = $6",
    )
    .bind(review.review_state_after_id)
    .bind(review.stability)
    .bind(review.difficulty)
    .bind(review.reviewed_at)
    .bind(review.next_due_at)
    .bind(review.flashcard_id)
    .execute(&mut *connection)
    .await?;

    sqlx::query(
        "INSERT INTO flashcard_reviews
            (flashcard_id, rating, stability, difficulty,
             review_state_before_id, review_state_after_id,
             reviewed_at, duration_ms, next_due_at)
         VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)",
    )
    .bind(review.flashcard_id)
    .bind(review.rating)
    .bind(review.stability)
    .bind(review.difficulty)
    .bind(review.review_state_before_id)
    .bind(review.review_state_after_id)
    .bind(review.reviewed_at)
    .bind(review.duration_ms)
    .bind(review.next_due_at)
    .execute(connection)
    .await?;

    Ok(())
}
