use crate::models::entity::EntityType;
use sqlx::{Postgres, Transaction};

pub async fn create_entity(
    tx: &mut Transaction<'_, Postgres>,
    entity_type: EntityType,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar("INSERT INTO entities (type) VALUES ($1) RETURNING id")
        .bind(entity_type as i32)
        .fetch_one(&mut **tx)
        .await
}

pub async fn update_entity(
    tx: &mut Transaction<'_, Postgres>,
    entity_id: i32,
) -> Result<(), sqlx::Error> {
    sqlx::query(
        "UPDATE entities
         SET updated_at = NOW(), version = version + 1
         WHERE id = $1",
    )
    .bind(entity_id)
    .execute(&mut **tx)
    .await?;

    Ok(())
}
