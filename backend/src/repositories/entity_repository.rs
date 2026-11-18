use crate::models::entity::EntityType;
use sqlx::PgConnection;

pub async fn create_entity(
    connection: &mut PgConnection,
    entity_type: EntityType,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar("INSERT INTO entities (type_id) VALUES ($1) RETURNING id")
        .bind(entity_type as i32)
        .fetch_one(connection)
        .await
}
