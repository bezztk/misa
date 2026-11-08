use crate::models::entity::EntityType;
use crate::repositories::entity_repository;
use sqlx::PgPool;

pub async fn create_topic(
    pool: &PgPool,
    parent_topic_entity_id: Option<i32>,
    name: &str,
    understanding_level_id: i32,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let entity_id = entity_repository::create_entity(&mut tx, EntityType::Topic).await?;

    sqlx::query(
        "INSERT INTO topics (entity_id, parent_topic_entity_id, name, understanding_level_id)
         VALUES ($1, $2, $3, $4)",
    )
    .bind(entity_id)
    .bind(parent_topic_entity_id)
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
    parent_topic_entity_id: Option<i32>,
    name: &str,
    understanding_level_id: i32,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE topics
         SET parent_topic_entity_id = $1, name = $2, understanding_level_id = $3
         WHERE entity_id = $4",
    )
    .bind(parent_topic_entity_id)
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
