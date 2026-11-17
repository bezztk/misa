use crate::models::system_task::{OPEN_SYSTEM_TASK_STATE_ID, TaskListItem, UpdateTaskRequest};
use sqlx::PgPool;

pub async fn get_tasks(pool: &PgPool) -> Result<Vec<TaskListItem>, sqlx::Error> {
    sqlx::query_as::<_, TaskListItem>(
        "SELECT id, name, pursuit_id, group_id, state_id, priority_id
         FROM systemtasks
         WHERE NOT EXISTS (
             SELECT 1 FROM systemtask_routines
             WHERE systemtask_id = systemtasks.id
         )
         ORDER BY id DESC",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_task(
    pool: &PgPool,
    name: &str,
    group_id: Option<i32>,
    priority_id: i32,
    pursuit_id: Option<i32>,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO systemtasks (name, group_id, state_id, priority_id, pursuit_id)
         VALUES ($1, $2, $3, $4, $5)
         RETURNING id",
    )
    .bind(name)
    .bind(group_id)
    .bind(OPEN_SYSTEM_TASK_STATE_ID)
    .bind(priority_id)
    .bind(pursuit_id)
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
    let mut transaction = pool.begin().await?;

    sqlx::query("DELETE FROM systemtask_sessions WHERE systemtask_id = $1")
        .bind(id)
        .execute(&mut *transaction)
        .await?;
    sqlx::query("DELETE FROM calendar_entries WHERE systemtask_id = $1")
        .bind(id)
        .execute(&mut *transaction)
        .await?;
    let result = sqlx::query("DELETE FROM systemtasks WHERE id = $1")
        .bind(id)
        .execute(&mut *transaction)
        .await?;

    transaction.commit().await?;

    Ok(result.rows_affected() > 0)
}
