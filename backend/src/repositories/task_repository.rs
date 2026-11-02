use crate::models::item_type::ItemType;
use crate::models::task::TaskDetails;
use crate::models::task_metadata::{TaskGroup, TaskMetadata, TaskStateRow};
use sqlx::PgPool;

pub async fn create_task(
    pool: &PgPool,
    name: &str,
    group_id: Option<i32>,
    description: Option<&str>,
) -> Result<i64, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let item_id: i64 = sqlx::query_scalar("INSERT INTO items (type) VALUES ($1) RETURNING id")
        .bind(ItemType::Task as i32)
        .fetch_one(&mut *tx)
        .await?;

    sqlx::query(
        "INSERT INTO tasks (item_id, group_id, name, description)
         VALUES ($1, $2, $3, $4)",
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

pub async fn get_task(pool: &PgPool, item_id: i64) -> Result<Option<TaskDetails>, sqlx::Error> {
    sqlx::query_as::<_, TaskDetails>(
        "SELECT
            i.id AS item_id,
            i.type AS item_type,
            i.created_at,
            i.updated_at,
            i.is_archived,
            i.version,
            t.group_id,
            t.state_id,
            t.name,
            t.description,
            t.completed_at
         FROM items i
         JOIN tasks t ON t.item_id = i.id
         WHERE i.id = $1",
    )
    .bind(item_id)
    .fetch_optional(pool)
    .await
}

pub async fn get_tasks(pool: &PgPool) -> Result<Vec<TaskDetails>, sqlx::Error> {
    sqlx::query_as::<_, TaskDetails>(
        "SELECT
            i.id AS item_id,
            i.type AS item_type,
            i.created_at,
            i.updated_at,
            i.is_archived,
            i.version,
            t.group_id,
            t.state_id,
            t.name,
            t.description,
            t.completed_at
         FROM items i
         JOIN tasks t ON t.item_id = i.id
         ORDER BY i.created_at DESC, i.id DESC",
    )
    .fetch_all(pool)
    .await
}

pub async fn task_state_exists(pool: &PgPool, state_id: i32) -> Result<bool, sqlx::Error> {
    sqlx::query_scalar("SELECT EXISTS(SELECT 1 FROM task_states WHERE id = $1)")
        .bind(state_id)
        .fetch_one(pool)
        .await
}

pub async fn task_group_exists(pool: &PgPool, group_id: i32) -> Result<bool, sqlx::Error> {
    sqlx::query_scalar("SELECT EXISTS(SELECT 1 FROM task_groups WHERE id = $1)")
        .bind(group_id)
        .fetch_one(pool)
        .await
}

pub async fn update_task(
    pool: &PgPool,
    item_id: i64,
    name: Option<&str>,
    state_id: Option<i32>,
    group_id: Option<i32>,
    description: Option<&str>,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let result = sqlx::query(
        "UPDATE tasks
         SET name = COALESCE($1, name),
             completed_at = CASE
                 WHEN $2::integer IS NULL OR $2 = state_id
                     THEN completed_at
                 WHEN $2 = 3 THEN NOW()
                 ELSE NULL
             END,
             state_id = COALESCE($2, state_id),
             group_id = COALESCE($3, group_id),
             description = COALESCE($4, description)
         WHERE item_id = $5",
    )
    .bind(name)
    .bind(state_id)
    .bind(group_id)
    .bind(description)
    .bind(item_id)
    .execute(&mut *tx)
    .await?;

    if result.rows_affected() == 0 {
        return Ok(false);
    }

    sqlx::query(
        "UPDATE items
         SET updated_at = NOW(),
             version = version + 1
         WHERE id = $1",
    )
    .bind(item_id)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;

    Ok(true)
}

pub async fn delete_task(pool: &PgPool, item_id: i64) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM items WHERE id = $1 AND type = $2")
        .bind(item_id)
        .bind(ItemType::Task as i32)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}

pub async fn get_task_metadata(pool: &PgPool) -> Result<TaskMetadata, sqlx::Error> {
    let groups = sqlx::query_as::<_, TaskGroup>("SELECT id, name FROM task_groups ORDER BY id")
        .fetch_all(pool)
        .await?;

    let states = sqlx::query_as::<_, TaskStateRow>("SELECT id, name FROM task_states ORDER BY id")
        .fetch_all(pool)
        .await?;

    Ok(TaskMetadata { groups, states })
}
