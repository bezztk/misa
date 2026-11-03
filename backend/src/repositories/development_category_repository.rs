use crate::models::development_category::DevelopmentCategory;
use crate::models::item_type::ItemType;
use crate::repositories::item_repository;
use sqlx::PgPool;

pub async fn create_development_category(
    pool: &PgPool,
    area_id: i32,
    name: &str,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let item_id =
        item_repository::create_item(&mut tx, ItemType::DevelopmentCategory).await?;

    sqlx::query(
        "INSERT INTO development_categories (item_id, area_id, name)
         VALUES ($1, $2, $3)",
    )
    .bind(item_id)
    .bind(area_id)
    .bind(name)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;

    Ok(item_id)
}

pub async fn get_development_category(
    pool: &PgPool,
    item_id: i32,
) -> Result<Option<DevelopmentCategory>, sqlx::Error> {
    sqlx::query_as(
        "SELECT item_id, area_id, name
         FROM development_categories
         WHERE item_id = $1",
    )
    .bind(item_id)
    .fetch_optional(pool)
    .await
}

pub async fn get_development_categories(
    pool: &PgPool,
) -> Result<Vec<DevelopmentCategory>, sqlx::Error> {
    sqlx::query_as(
        "SELECT item_id, area_id, name
         FROM development_categories
         ORDER BY name, item_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn update_development_category(
    pool: &PgPool,
    item_id: i32,
    name: Option<&str>,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let result = sqlx::query(
        "UPDATE development_categories
         SET name = COALESCE($1, name)
         WHERE item_id = $2",
    )
    .bind(name)
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

pub async fn delete_development_category(
    pool: &PgPool,
    item_id: i32,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let result = sqlx::query("DELETE FROM development_categories WHERE item_id = $1")
        .bind(item_id)
        .execute(&mut *tx)
        .await?;

    if result.rows_affected() == 0 {
        tx.rollback().await?;
        return Ok(false);
    }

    sqlx::query("DELETE FROM items WHERE id = $1 AND type = $2")
        .bind(item_id)
        .bind(ItemType::DevelopmentCategory as i32)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;

    Ok(true)
}
