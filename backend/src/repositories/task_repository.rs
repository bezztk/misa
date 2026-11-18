use crate::models::entity::EntityType;
use crate::models::system_task::{OPEN_SYSTEM_TASK_STATE_ID, TaskListItem, UpdateTaskRequest};
use crate::repositories::entity_repository;
use sqlx::PgPool;

pub async fn get_tasks(pool: &PgPool) -> Result<Vec<TaskListItem>, sqlx::Error> {
    sqlx::query_as::<_, TaskListItem>(
        "SELECT entity_id AS id, name, domain_node_id, group_id, state_id, priority_id
         FROM systemtasks
         ORDER BY entity_id DESC",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_task(
    pool: &PgPool,
    name: &str,
    group_id: Option<i32>,
    priority_id: i32,
    domain_node_id: Option<i32>,
) -> Result<i32, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let entity_id =
        entity_repository::create_entity(&mut transaction, EntityType::SystemTask).await?;
    sqlx::query(
        "INSERT INTO systemtasks
            (entity_id, name, group_id, state_id, priority_id, domain_node_id)
         VALUES ($1, $2, $3, $4, $5, $6)",
    )
    .bind(entity_id)
    .bind(name)
    .bind(group_id)
    .bind(OPEN_SYSTEM_TASK_STATE_ID)
    .bind(priority_id)
    .bind(domain_node_id)
    .execute(&mut *transaction)
    .await?;
    transaction.commit().await?;
    Ok(entity_id)
}

pub async fn update_task(
    pool: &PgPool,
    id: i32,
    task: &UpdateTaskRequest,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE systemtasks
         SET name = COALESCE($1, name),
             state_id = COALESCE($2, state_id),
             priority_id = COALESCE($3, priority_id)
         WHERE entity_id = $4",
    )
    .bind(task.name.as_deref())
    .bind(task.state_id)
    .bind(task.priority_id)
    .bind(id)
    .execute(&mut *transaction)
    .await?;

    if result.rows_affected() > 0 {
        sqlx::query("UPDATE entities SET version = version + 1, updated_at = NOW() WHERE id = $1")
            .bind(id)
            .execute(&mut *transaction)
            .await?;
    }
    transaction.commit().await?;

    Ok(result.rows_affected() > 0)
}

pub async fn delete_task(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;

    sqlx::query(
        "DELETE FROM systemtask_sessions
         WHERE systemtask_id = $1
            OR routine_execution_id IN (
                SELECT execution.id
                FROM systemtask_routine_executions execution
                JOIN systemtask_routines routine ON routine.id = execution.routine_id
                WHERE routine.systemtask_id = $1
            )",
    )
    .bind(id)
    .execute(&mut *transaction)
    .await?;
    sqlx::query(
        "DELETE FROM entities
         WHERE id IN (
             SELECT entry.entity_id
             FROM calendar_entries entry
             WHERE entry.systemtask_id = $1
                OR entry.routine_execution_id IN (
                    SELECT execution.id
                    FROM systemtask_routine_executions execution
                    JOIN systemtask_routines routine ON routine.id = execution.routine_id
                    WHERE routine.systemtask_id = $1
                )
         )",
    )
    .bind(id)
    .execute(&mut *transaction)
    .await?;
    let result = sqlx::query("DELETE FROM entities WHERE id = $1")
        .bind(id)
        .execute(&mut *transaction)
        .await?;

    transaction.commit().await?;

    Ok(result.rows_affected() > 0)
}
