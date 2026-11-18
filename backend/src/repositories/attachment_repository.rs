use crate::models::attachment::{Attachment, CreateAttachmentRequest};
use crate::models::entity::EntityType;
use crate::repositories::entity_repository;
use sqlx::PgPool;

pub async fn get_attachments(
    pool: &PgPool,
    domain_node_id: i32,
) -> Result<Vec<Attachment>, sqlx::Error> {
    sqlx::query_as::<_, Attachment>(
        "SELECT a.entity_id AS id, a.domain_node_id, a.original_name, a.storage_name,
                a.size_bytes, e.created_at
         FROM attachments a
         JOIN entities e ON e.id = a.entity_id
         WHERE a.domain_node_id = $1
         ORDER BY e.created_at, a.entity_id",
    )
    .bind(domain_node_id)
    .fetch_all(pool)
    .await
}

pub async fn create_attachment(
    pool: &PgPool,
    domain_node_id: i32,
    attachment: &CreateAttachmentRequest,
) -> Result<Option<Attachment>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let entity_id =
        entity_repository::create_entity(&mut transaction, EntityType::Attachment).await?;
    let result = sqlx::query_as::<_, Attachment>(
        "WITH inserted AS (
             INSERT INTO attachments
                 (entity_id, domain_node_id, original_name, storage_name, size_bytes)
             SELECT $1, entity_id, $3, $4, $5
             FROM domain_nodes
             WHERE entity_id = $2
             RETURNING entity_id, domain_node_id, original_name, storage_name, size_bytes
         )
         SELECT inserted.entity_id AS id, inserted.domain_node_id,
                inserted.original_name, inserted.storage_name, inserted.size_bytes,
                entities.created_at
         FROM inserted
         JOIN entities ON entities.id = inserted.entity_id",
    )
    .bind(entity_id)
    .bind(domain_node_id)
    .bind(&attachment.original_name)
    .bind(&attachment.storage_name)
    .bind(attachment.size_bytes)
    .fetch_optional(&mut *transaction)
    .await?;
    if result.is_none() {
        return Ok(None);
    }
    transaction.commit().await?;
    Ok(result)
}

pub async fn delete_attachment(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM entities WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
