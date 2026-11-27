use serde_json::Value;
use sqlx::PgPool;
use uuid::Uuid;

use crate::models::source::{Favicon, Source};

pub async fn get_sources(pool: &PgPool) -> Result<Vec<Source>, sqlx::Error> {
    sqlx::query_as::<_, Source>(
        "SELECT sources.*, resource_types.display_name AS resource_type
         FROM sources
         JOIN resource_types ON resource_types.id = sources.resource_type_id
         ORDER BY sources.created_at DESC, sources.id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_source(
    pool: &PgPool,
    resource_type: &str,
    title: Option<&str>,
    url: &str,
    description: Option<&str>,
    metadata: &Value,
    favicon: Option<&Favicon>,
) -> Result<Uuid, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO sources (resource_type_id, title, url, description, metadata, favicon, favicon_mime_type)
         VALUES ((SELECT id FROM resource_types WHERE name = $1), $2, $3, $4, $5, $6, $7)
         RETURNING id",
    )
    .bind(resource_type)
    .bind(title)
    .bind(url)
    .bind(description)
    .bind(metadata)
    .bind(favicon.map(|icon| icon.data.as_slice()))
    .bind(favicon.map(|icon| icon.mime_type.as_str()))
    .fetch_one(pool)
    .await
}

pub async fn delete_source(pool: &PgPool, id: Uuid) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM sources WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() > 0)
}

pub async fn update_title(pool: &PgPool, id: Uuid, title: &str) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("UPDATE sources SET title = $2, updated_at = now() WHERE id = $1")
        .bind(id)
        .bind(title)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() > 0)
}
