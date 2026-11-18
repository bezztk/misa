use crate::models::document::Document;
use sqlx::PgPool;

pub async fn create_document(pool: &PgPool, node_id: i32) -> Result<Option<Document>, sqlx::Error> {
    sqlx::query_as::<_, Document>(
        "INSERT INTO documents (domain_node_id)
         SELECT entity_id FROM domain_nodes WHERE entity_id = $1
         RETURNING domain_node_id, content, completeness_level_id, flashcard_status_id,
                   verification_status_id, verification_level_id",
    )
    .bind(node_id)
    .fetch_optional(pool)
    .await
}

pub async fn get_document(pool: &PgPool, node_id: i32) -> Result<Option<Document>, sqlx::Error> {
    sqlx::query_as::<_, Document>(
        "SELECT domain_node_id, content, completeness_level_id, flashcard_status_id,
                verification_status_id, verification_level_id
         FROM documents
         WHERE domain_node_id = $1",
    )
    .bind(node_id)
    .fetch_optional(pool)
    .await
}

pub async fn update_document(
    pool: &PgPool,
    node_id: i32,
    content: &str,
) -> Result<Option<Document>, sqlx::Error> {
    sqlx::query_as::<_, Document>(
        "UPDATE documents
         SET content = $2
         WHERE domain_node_id = $1
         RETURNING domain_node_id, content, completeness_level_id, flashcard_status_id,
                   verification_status_id, verification_level_id",
    )
    .bind(node_id)
    .bind(content)
    .fetch_optional(pool)
    .await
}

pub async fn delete_document(pool: &PgPool, node_id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM documents WHERE domain_node_id = $1")
        .bind(node_id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
