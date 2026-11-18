use crate::models::entity::EntityType;
use crate::models::flashcard::{EditableFlashcard, FlashcardQueueContext, FlashcardReviewContext};
use crate::repositories::entity_repository;
use chrono::{DateTime, Utc};
use sqlx::{PgConnection, PgPool};

const DEFAULT_RETENTION_CLASS_ID: i32 = 2;

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
    let mut transaction = pool.begin().await?;
    let entity_id =
        entity_repository::create_entity(&mut transaction, EntityType::Flashcard).await?;
    let flashcard = sqlx::query_as::<_, EditableFlashcard>(
        "INSERT INTO flashcards
            (entity_id, domain_node_id, front, back, retention_class_id)
         VALUES ($1, $2, $3, $4, $5)
         RETURNING entity_id AS id, front, back",
    )
    .bind(entity_id)
    .bind(domain_node_id)
    .bind(front)
    .bind(back)
    .bind(DEFAULT_RETENTION_CLASS_ID)
    .fetch_one(&mut *transaction)
    .await?;
    transaction.commit().await?;
    Ok(flashcard)
}

pub async fn get_flashcards(
    pool: &PgPool,
    domain_node_id: i32,
) -> Result<Vec<EditableFlashcard>, sqlx::Error> {
    sqlx::query_as::<_, EditableFlashcard>(
        "SELECT entity_id AS id, front, back
         FROM flashcards
         WHERE domain_node_id = $1
         ORDER BY entity_id",
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
    let mut transaction = pool.begin().await?;
    let flashcard = sqlx::query_as::<_, EditableFlashcard>(
        "UPDATE flashcards
         SET front = $2, back = $3
         WHERE entity_id = $1
         RETURNING entity_id AS id, front, back",
    )
    .bind(id)
    .bind(front)
    .bind(back)
    .fetch_optional(&mut *transaction)
    .await?;
    if flashcard.is_some() {
        sqlx::query("UPDATE entities SET version = version + 1, updated_at = NOW() WHERE id = $1")
            .bind(id)
            .execute(&mut *transaction)
            .await?;
    }
    transaction.commit().await?;
    Ok(flashcard)
}

pub async fn get_due_flashcard_contexts(
    pool: &PgPool,
) -> Result<Vec<FlashcardQueueContext>, sqlx::Error> {
    sqlx::query_as::<_, FlashcardQueueContext>(
        "SELECT
            f.entity_id AS id,
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
                WHERE fr.flashcard_id = f.entity_id
                ORDER BY fr.reviewed_at DESC, fr.id DESC
                LIMIT 1
            ) AS last_rating
         FROM flashcards f
         JOIN flashcard_retention_classes r ON r.id = f.retention_class_id
         WHERE f.due_at IS NULL OR f.due_at <= NOW()
         ORDER BY f.due_at NULLS FIRST, f.entity_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn delete_flashcard(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM entities WHERE id = $1")
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
                 WHERE fr.flashcard_id = f.entity_id
                 ORDER BY fr.reviewed_at DESC, fr.id DESC
                 LIMIT 1
             ) AS last_rating
         FROM flashcards f
         JOIN flashcard_retention_classes r ON r.id = f.retention_class_id
         WHERE f.entity_id = $1
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
         WHERE entity_id = $6",
    )
    .bind(review.review_state_after_id)
    .bind(review.stability)
    .bind(review.difficulty)
    .bind(review.reviewed_at)
    .bind(review.next_due_at)
    .bind(review.flashcard_id)
    .execute(&mut *connection)
    .await?;

    sqlx::query("UPDATE entities SET version = version + 1, updated_at = NOW() WHERE id = $1")
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
