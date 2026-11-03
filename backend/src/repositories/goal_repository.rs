use crate::models::goal::Goal;
use crate::models::item_type::ItemType;
use crate::repositories::item_repository;
use chrono::{DateTime, Utc};
use sqlx::PgPool;

pub async fn create_goal(
    pool: &PgPool,
    area_id: i32,
    name: &str,
    description: Option<&str>,
    due_at: Option<DateTime<Utc>>,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let item_id = item_repository::create_item(&mut tx, ItemType::Goal).await?;

    sqlx::query(
        "INSERT INTO goals (item_id, area_id, name, description, due_at)
         VALUES ($1, $2, $3, $4, $5)",
    )
    .bind(item_id)
    .bind(area_id)
    .bind(name)
    .bind(description)
    .bind(due_at)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;

    Ok(item_id)
}

pub async fn get_goal(pool: &PgPool, item_id: i32) -> Result<Option<Goal>, sqlx::Error> {
    sqlx::query_as(
        "SELECT item_id, area_id, name, description, is_completed, completed_at, due_at
         FROM goals
         WHERE item_id = $1",
    )
    .bind(item_id)
    .fetch_optional(pool)
    .await
}

pub async fn get_goals(pool: &PgPool) -> Result<Vec<Goal>, sqlx::Error> {
    sqlx::query_as(
        "SELECT item_id, area_id, name, description, is_completed, completed_at, due_at
         FROM goals
         ORDER BY name, item_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn update_goal(
    pool: &PgPool,
    item_id: i32,
    name: Option<&str>,
    description: Option<&str>,
    is_completed: Option<bool>,
    due_at: Option<DateTime<Utc>>,
    clear_due_at: bool,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let result = sqlx::query(
        "UPDATE goals
         SET name = COALESCE($1, name),
             description = COALESCE($2, description),
             completed_at = CASE
                 WHEN $3::BOOLEAN IS NULL OR $3 = is_completed THEN completed_at
                 WHEN $3 THEN NOW()
                 ELSE NULL
             END,
             is_completed = COALESCE($3, is_completed),
             due_at = CASE WHEN $5 THEN NULL ELSE COALESCE($4, due_at) END
         WHERE item_id = $6",
    )
    .bind(name)
    .bind(description)
    .bind(is_completed)
    .bind(due_at)
    .bind(clear_due_at)
    .bind(item_id)
    .execute(&mut *tx)
    .await?;

    if result.rows_affected() == 0 {
        tx.rollback().await?;
        return Ok(false);
    }

    item_repository::update_item(&mut tx, i64::from(item_id)).await?;
    tx.commit().await?;

    Ok(true)
}

pub async fn delete_goal(pool: &PgPool, item_id: i32) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let result = sqlx::query("DELETE FROM goals WHERE item_id = $1")
        .bind(item_id)
        .execute(&mut *tx)
        .await?;

    if result.rows_affected() == 0 {
        tx.rollback().await?;
        return Ok(false);
    }

    sqlx::query("DELETE FROM items WHERE id = $1 AND type = $2")
        .bind(item_id)
        .bind(ItemType::Goal as i32)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;

    Ok(true)
}
