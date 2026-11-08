use crate::models::entity::EntityType;
use crate::repositories::entity_repository;
use sqlx::PgPool;

pub async fn create_topic(
    pool: &PgPool,
    name: &str,
    understanding_level_id: i32,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let entity_id = entity_repository::create_entity(&mut tx, EntityType::Topic).await?;

    sqlx::query(
        "INSERT INTO topics (entity_id, name, understanding_level_id)
         VALUES ($1, $2, $3)",
    )
    .bind(entity_id)
    .bind(name)
    .bind(understanding_level_id)
    .execute(&mut *tx)
    .await?;

    let document_id = entity_repository::create_entity(&mut tx, EntityType::Document).await?;
    sqlx::query("INSERT INTO documents (entity_id, topic_id) VALUES ($1, $2)")
        .bind(document_id)
        .bind(entity_id)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;
    Ok(entity_id)
}

pub async fn update_topic(
    pool: &PgPool,
    entity_id: i32,
    name: &str,
    understanding_level_id: i32,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE topics
         SET name = $1, understanding_level_id = $2
         WHERE entity_id = $3",
    )
    .bind(name)
    .bind(understanding_level_id)
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

pub async fn delete_topic(pool: &PgPool, entity_id: i32) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let document_id: Option<i32> =
        sqlx::query_scalar("SELECT entity_id FROM documents WHERE topic_id = $1")
            .bind(entity_id)
            .fetch_optional(&mut *tx)
            .await?;

    if let Some(document_id) = document_id {
        sqlx::query("DELETE FROM document_revisions WHERE document_id = $1")
            .bind(document_id)
            .execute(&mut *tx)
            .await?;
        sqlx::query("DELETE FROM documents WHERE entity_id = $1")
            .bind(document_id)
            .execute(&mut *tx)
            .await?;
        sqlx::query("DELETE FROM entities WHERE id = $1 AND type = $2")
            .bind(document_id)
            .bind(EntityType::Document as i32)
            .execute(&mut *tx)
            .await?;
    }

    let result = sqlx::query("DELETE FROM topics WHERE entity_id = $1")
        .bind(entity_id)
        .execute(&mut *tx)
        .await?;

    if result.rows_affected() == 0 {
        return Ok(false);
    }

    sqlx::query("DELETE FROM entities WHERE id = $1 AND type = $2")
        .bind(entity_id)
        .bind(EntityType::Topic as i32)
        .execute(&mut *tx)
        .await?;
    tx.commit().await?;
    Ok(true)
}
