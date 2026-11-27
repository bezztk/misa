use crate::models::context::{Context, SaveContextRequest};
use sqlx::PgPool;

pub async fn get_contexts(pool: &PgPool) -> Result<Vec<Context>, sqlx::Error> {
    sqlx::query_as(
        "SELECT context.id, context.title, context.icon, context.color,
                (SELECT COUNT(*) FROM context_entities WHERE context_id = context.id)
                    AS entity_count
         FROM contexts context
         ORDER BY context.title, context.id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_context(
    pool: &PgPool,
    context: &SaveContextRequest,
) -> Result<Context, sqlx::Error> {
    sqlx::query_as(
        "INSERT INTO contexts (title, icon, color) VALUES ($1, $2, $3)
         RETURNING id, title, icon, color, 0::bigint AS entity_count",
    )
    .bind(context.title.trim())
    .bind(context.icon.as_deref())
    .bind(context.color.as_deref())
    .fetch_one(pool)
    .await
}

pub async fn update_context(
    pool: &PgPool,
    id: i32,
    context: &SaveContextRequest,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("UPDATE contexts SET title = $2, icon = $3, color = $4 WHERE id = $1")
        .bind(id)
        .bind(context.title.trim())
        .bind(context.icon.as_deref())
        .bind(context.color.as_deref())
        .execute(pool)
        .await?;
    Ok(result.rows_affected() > 0)
}

pub async fn delete_context(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM contexts WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() > 0)
}
