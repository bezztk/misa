use crate::models::flashcard::Flashcard;
use sqlx::PgPool;

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
