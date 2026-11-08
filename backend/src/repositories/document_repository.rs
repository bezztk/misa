use crate::repositories::entity_repository;
use chrono::{DateTime, Utc};
use sqlx::PgPool;

pub async fn update_document(
    pool: &PgPool,
    entity_id: i32,
    content: &str,
    completeness_level_id: Option<i32>,
    flashcard_status_id: i32,
    verification_status_id: i32,
    verification_level_id: Option<i32>,
    last_verified_at: Option<DateTime<Utc>>,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE documents
         SET content = $1,
             completeness_level_id = $2,
             flashcard_status_id = $3,
             verification_status_id = $4,
             verification_level_id = $5,
             last_verified_at = $6
         WHERE entity_id = $7",
    )
    .bind(content)
    .bind(completeness_level_id)
    .bind(flashcard_status_id)
    .bind(verification_status_id)
    .bind(verification_level_id)
    .bind(last_verified_at)
    .bind(entity_id)
    .execute(&mut *tx)
    .await?;

    if result.rows_affected() == 0 {
        return Ok(false);
    }

    entity_repository::update_entity(&mut tx, entity_id).await?;
    tx.commit().await?;
    Ok(true)
}
