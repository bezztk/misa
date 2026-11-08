use crate::repositories::entity_repository;
use chrono::{DateTime, Utc};
use sqlx::PgPool;

use crate::models::document::Document;

pub async fn get_document(pool: &PgPool, entity_id: i32) -> Result<Option<Document>, sqlx::Error> {
    sqlx::query_as::<_, Document>(
        "SELECT
            d.entity_id,
            e.created_at,
            e.updated_at,
            e.version,
            d.topic_id,
            d.content,
            d.completeness_level_id,
            d.flashcard_status_id,
            d.verification_status_id,
            d.verification_level_id,
            d.last_verified_at
         FROM documents d
         JOIN entities e ON e.id = d.entity_id
         WHERE d.entity_id = $1
           AND NOT e.is_archived
           AND NOT e.is_deleted",
    )
    .bind(entity_id)
    .fetch_optional(pool)
    .await
}

pub async fn get_document_by_topic(
    pool: &PgPool,
    topic_id: i32,
) -> Result<Option<Document>, sqlx::Error> {
    sqlx::query_as::<_, Document>(
        "SELECT
            d.entity_id,
            e.created_at,
            e.updated_at,
            e.version,
            d.topic_id,
            d.content,
            d.completeness_level_id,
            d.flashcard_status_id,
            d.verification_status_id,
            d.verification_level_id,
            d.last_verified_at
         FROM documents d
         JOIN entities e ON e.id = d.entity_id
         WHERE d.topic_id = $1
           AND NOT e.is_archived
           AND NOT e.is_deleted",
    )
    .bind(topic_id)
    .fetch_optional(pool)
    .await
}

pub async fn create_document(pool: &PgPool, topic_id: i32) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let entity_id =
        entity_repository::create_entity(&mut tx, crate::models::entity::EntityType::Document)
            .await?;

    sqlx::query("INSERT INTO documents (entity_id, topic_id) VALUES ($1, $2)")
        .bind(entity_id)
        .bind(topic_id)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;
    Ok(entity_id)
}

pub async fn update_document_content(
    pool: &PgPool,
    entity_id: i32,
    content: &str,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let result = sqlx::query("UPDATE documents SET content = $1 WHERE entity_id = $2")
        .bind(content)
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
