use crate::models::comment::Comment;
use sqlx::PgPool;

pub async fn get_comments_for_entities(
    pool: &PgPool,
    entity_ids: &[i32],
) -> Result<Vec<Comment>, sqlx::Error> {
    sqlx::query_as::<_, Comment>(
        "SELECT id, entity_id, content, created_at, updated_at
         FROM comments
         WHERE entity_id = ANY($1)
         ORDER BY entity_id, created_at, id",
    )
    .bind(entity_ids)
    .fetch_all(pool)
    .await
}

pub async fn create_comment(
    pool: &PgPool,
    entity_id: i32,
    content: &str,
) -> Result<Comment, sqlx::Error> {
    sqlx::query_as::<_, Comment>(
        "INSERT INTO comments (entity_id, content)
         VALUES ($1, $2)
         RETURNING id, entity_id, content, created_at, updated_at",
    )
    .bind(entity_id)
    .bind(content)
    .fetch_one(pool)
    .await
}

pub async fn update_comment(
    pool: &PgPool,
    id: i32,
    content: &str,
) -> Result<Option<Comment>, sqlx::Error> {
    sqlx::query_as::<_, Comment>(
        "UPDATE comments
         SET content = $2, updated_at = NOW()
         WHERE id = $1
         RETURNING id, entity_id, content, created_at, updated_at",
    )
    .bind(id)
    .bind(content)
    .fetch_optional(pool)
    .await
}

pub async fn delete_comment(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM comments WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
