use sqlx::{Connection, PgConnection};
use crate::models::item_type::ItemType;
use crate::models::task::Task;

pub async fn create_task(
    connection: &mut PgConnection,
    name: &str,
    group_id: Option<i32>,
    description: Option<&str>,
) -> Result<i64, sqlx::Error> {

    let mut tx = connection.begin().await?;

    let item_id: i64 = sqlx::query_scalar(
        "INSERT INTO items (type) VALUES ($1) RETURNING id"
    )
        .bind(ItemType::Task as i32)
        .fetch_one(&mut *tx)
        .await?;

    sqlx::query(
        "INSERT INTO tasks (item_id, group_id, name, description)
        VALUES ($1, $2, $3, $4)"
    )
        .bind(item_id)
        .bind(group_id)
        .bind(name)
        .bind(description)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;

    Ok(item_id)
}

pub async fn get_task(
    connection: &mut PgConnection,
    item_id: i64
) -> Result<Option<Task>, sqlx::error::Error> {
    sqlx::query_as::<_, Task>("SELECT * FROM tasks WHERE item_id = $1")
    .bind(item_id)
    .fetch_optional(connection)
    .await
}

pub async fn update_task(
    connection: &mut PgConnection,
    item_id: i64,
    name: &str,
    state_id: i32,
) -> Result<bool, sqlx::error::Error> {

    let mut tx = connection.begin().await?;

    let result = sqlx::query(
        "UPDATE tasks
         SET name = $1,
             state_id = $2,
             completed_at = CASE
                 WHEN $2 = 3 THEN
                     CASE
                         WHEN state_id = 3 THEN completed_at
                         ELSE NOW()
                     END
                 ELSE NULL
             END
         WHERE item_id = $3"
    )
        .bind(name)
        .bind(state_id)
        .bind(item_id)
        .execute(&mut *tx)
        .await?;

    if result.rows_affected() == 0 {
        return Ok(false);
    }

    // Update Item-Metadata
    sqlx::query(
        "UPDATE items
         SET updated_at = NOW(),
             version = version + 1
         WHERE id = $1"
    )
        .bind(item_id)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;

    Ok(true)
}

pub async fn delete_task(
    connection: &mut PgConnection,
    item_id: i64,
) -> Result<bool, sqlx::Error> {

    let result = sqlx::query(
        "DELETE FROM items
         WHERE id = $1 AND type = $2"
    )
        .bind(item_id)
        .bind(ItemType::Task as i32)
        .execute(connection)
        .await?;

    Ok(result.rows_affected() > 0)
}