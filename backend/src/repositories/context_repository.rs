use crate::models::context::Context;
use sqlx::PgPool;

pub async fn get_contexts(pool: &PgPool) -> Result<Vec<Context>, sqlx::Error> {
    sqlx::query_as("SELECT id, title FROM contexts ORDER BY title, id")
        .fetch_all(pool)
        .await
}

pub async fn create_context(pool: &PgPool, title: &str) -> Result<Context, sqlx::Error> {
    sqlx::query_as("INSERT INTO contexts (title) VALUES ($1) RETURNING id, title")
        .bind(title)
        .fetch_one(pool)
        .await
}

pub async fn rename_context(pool: &PgPool, id: i32, title: &str) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("UPDATE contexts SET title = $2 WHERE id = $1")
        .bind(id)
        .bind(title)
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
