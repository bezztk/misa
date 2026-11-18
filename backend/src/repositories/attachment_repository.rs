use crate::models::attachment::{Attachment, CreateAttachmentRequest};
use sqlx::PgPool;

pub async fn get_attachments(
    pool: &PgPool,
    domain_node_id: i32,
) -> Result<Vec<Attachment>, sqlx::Error> {
    sqlx::query_as::<_, Attachment>(
        "SELECT id, domain_node_id, original_name, storage_name, size_bytes, created_at
         FROM attachments
         WHERE domain_node_id = $1
         ORDER BY created_at, id",
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
    sqlx::query_as::<_, Attachment>(
        "INSERT INTO attachments (domain_node_id, original_name, storage_name, size_bytes)
         SELECT id, $2, $3, $4
         FROM domain_nodes
         WHERE id = $1
         RETURNING id, domain_node_id, original_name, storage_name, size_bytes, created_at",
    )
    .bind(domain_node_id)
    .bind(&attachment.original_name)
    .bind(&attachment.storage_name)
    .bind(attachment.size_bytes)
    .fetch_optional(pool)
    .await
}

pub async fn delete_attachment(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM attachments WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
