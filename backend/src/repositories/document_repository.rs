use crate::models::document::{Document, DocumentVersion};
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
    let mut transaction = pool.begin().await?;
    let Some(mut document) = sqlx::query_as::<_, Document>(
        "SELECT domain_node_id, content, completeness_level_id, flashcard_status_id,
                verification_status_id, verification_level_id
         FROM documents
         WHERE domain_node_id = $1
         FOR UPDATE",
    )
    .bind(node_id)
    .fetch_optional(&mut *transaction)
    .await?
    else {
        return Ok(None);
    };

    if document.content == content {
        transaction.commit().await?;
        return Ok(Some(document));
    }

    sqlx::query("UPDATE documents SET content = $2 WHERE domain_node_id = $1")
        .bind(node_id)
        .bind(content)
        .execute(&mut *transaction)
        .await?;

    let latest_version = sqlx::query_as::<_, (i32, bool)>(
        "SELECT version, window_started_at + INTERVAL '60 seconds' <= NOW()
         FROM document_versions
         WHERE domain_node_id = $1
         ORDER BY version DESC
         LIMIT 1
         FOR UPDATE",
    )
    .bind(node_id)
    .fetch_optional(&mut *transaction)
    .await?;

    match latest_version {
        Some((version, true)) => {
            sqlx::query(
                "INSERT INTO document_versions (domain_node_id, version, content)
                 VALUES ($1, $2, $3)",
            )
            .bind(node_id)
            .bind(version + 1)
            .bind(content)
            .execute(&mut *transaction)
            .await?;
        }
        Some((version, false)) => {
            sqlx::query(
                "UPDATE document_versions
                 SET content = $3, captured_at = NOW()
                 WHERE domain_node_id = $1 AND version = $2",
            )
            .bind(node_id)
            .bind(version)
            .bind(content)
            .execute(&mut *transaction)
            .await?;
        }
        None => {
            sqlx::query(
                "INSERT INTO document_versions (domain_node_id, version, content)
                 VALUES ($1, 1, $2)",
            )
            .bind(node_id)
            .bind(content)
            .execute(&mut *transaction)
            .await?;
        }
    }

    document.content = content.to_owned();
    transaction.commit().await?;
    Ok(Some(document))
}

pub async fn get_document_versions(
    pool: &PgPool,
    node_id: i32,
) -> Result<Vec<DocumentVersion>, sqlx::Error> {
    sqlx::query_as::<_, DocumentVersion>(
        "SELECT version, content, captured_at
         FROM document_versions
         WHERE domain_node_id = $1
         ORDER BY version DESC",
    )
    .bind(node_id)
    .fetch_all(pool)
    .await
}

pub async fn restore_document_version(
    pool: &PgPool,
    node_id: i32,
    version: i32,
) -> Result<Option<Document>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let Some(mut document) = sqlx::query_as::<_, Document>(
        "SELECT domain_node_id, content, completeness_level_id, flashcard_status_id,
                verification_status_id, verification_level_id
         FROM documents
         WHERE domain_node_id = $1
         FOR UPDATE",
    )
    .bind(node_id)
    .fetch_optional(&mut *transaction)
    .await?
    else {
        return Ok(None);
    };

    let Some(restored_content) = sqlx::query_scalar::<_, String>(
        "SELECT content
         FROM document_versions
         WHERE domain_node_id = $1 AND version = $2",
    )
    .bind(node_id)
    .bind(version)
    .fetch_optional(&mut *transaction)
    .await?
    else {
        return Ok(None);
    };

    let next_version = sqlx::query_scalar::<_, i32>(
        "SELECT COALESCE(MAX(version), 0) + 1
         FROM document_versions
         WHERE domain_node_id = $1",
    )
    .bind(node_id)
    .fetch_one(&mut *transaction)
    .await?;

    sqlx::query("UPDATE documents SET content = $2 WHERE domain_node_id = $1")
        .bind(node_id)
        .bind(&restored_content)
        .execute(&mut *transaction)
        .await?;
    sqlx::query(
        "INSERT INTO document_versions (domain_node_id, version, content)
         VALUES ($1, $2, $3)",
    )
    .bind(node_id)
    .bind(next_version)
    .bind(&restored_content)
    .execute(&mut *transaction)
    .await?;

    document.content = restored_content;
    transaction.commit().await?;
    Ok(Some(document))
}

pub async fn delete_document(pool: &PgPool, node_id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM documents WHERE domain_node_id = $1")
        .bind(node_id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
