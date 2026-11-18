use crate::models::entity::EntityType;
use serde_json::Value;
use sqlx::{PgConnection, Postgres, Transaction};

pub async fn create_entity(
    connection: &mut PgConnection,
    entity_type: EntityType,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar("INSERT INTO entities (type_id) VALUES ($1) RETURNING id")
        .bind(entity_type as i32)
        .fetch_one(connection)
        .await
}

pub async fn create_field_version(
    transaction: &mut Transaction<'_, Postgres>,
    entity_id: i32,
    entity_type: EntityType,
    field_type_id: i32,
    old_value: Value,
    new_value: Value,
) -> Result<(), sqlx::Error> {
    sqlx::query_scalar::<_, i32>(
        "WITH updated_entity AS (
             UPDATE entities
             SET version = version + 1, updated_at = NOW()
             WHERE id = $1 AND type_id = $2
             RETURNING version
         )
         INSERT INTO entity_field_versions
             (entity_id, version, field_id, old_value, new_value)
         SELECT $1, updated_entity.version, entity_fields.id, $4, $5
         FROM updated_entity
         JOIN entity_fields
           ON entity_fields.entity_type_id = $2
          AND entity_fields.field_type_id = $3
         RETURNING version",
    )
    .bind(entity_id)
    .bind(entity_type as i32)
    .bind(field_type_id)
    .bind(old_value)
    .bind(new_value)
    .fetch_one(&mut **transaction)
    .await?;

    Ok(())
}
