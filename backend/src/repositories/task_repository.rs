use crate::models::system_task::{SystemTaskStateId, TaskListItem, UpdateTaskRequest};
use sqlx::PgPool;

pub async fn get_tasks(pool: &PgPool) -> Result<Vec<TaskListItem>, sqlx::Error> {
    sqlx::query_as::<_, TaskListItem>(
        "SELECT id, name, group_id, state_id, priority_id
         FROM systemtasks
         ORDER BY id DESC",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_task(
    pool: &PgPool,
    name: &str,
    group_id: i32,
    priority_id: i32,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO systemtasks (name, group_id, state_id, priority_id)
         VALUES ($1, $2, $3, $4)
         RETURNING id",
    )
    .bind(name)
    .bind(group_id)
    .bind(SystemTaskStateId::Open as i32)
    .bind(priority_id)
    .fetch_one(pool)
    .await
}

pub async fn update_task(
    pool: &PgPool,
    id: i32,
    task: &UpdateTaskRequest,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE systemtasks
         SET name = COALESCE($1, name),
             state_id = COALESCE($2, state_id),
             priority_id = COALESCE($3, priority_id)
         WHERE id = $4",
    )
    .bind(task.name.as_deref())
    .bind(task.state_id)
    .bind(task.priority_id)
    .bind(id)
    .execute(pool)
    .await?;

    Ok(result.rows_affected() > 0)
}

pub async fn delete_task(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM systemtasks WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
