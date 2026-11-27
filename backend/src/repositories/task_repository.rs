use crate::models::entity::EntityType;
use crate::models::system_task::{
    OPEN_SYSTEM_TASK_STATE_ID, TaskField, TaskFieldUpdate, TaskListItem,
};
use crate::repositories::{comment_repository, entity_repository};
use chrono::{DateTime, Utc};
use serde_json::json;
use sqlx::{PgConnection, PgPool};

pub async fn get_tasks(pool: &PgPool) -> Result<Vec<TaskListItem>, sqlx::Error> {
    let mut tasks = sqlx::query_as::<_, TaskListItem>(
        "SELECT task.entity_id AS id, task.name, task.domain_node_id, node.name AS domain_node_name,
                task.state_id, task.priority_id, task.due_at,
                COALESCE(planning.total, 0) AS planning_count,
                ARRAY(
                    SELECT context_id
                    FROM context_entities
                    WHERE entity_id = task.entity_id
                    ORDER BY context_id
                ) AS context_ids
         FROM systemtasks task
         LEFT JOIN domain_nodes node ON node.entity_id = task.domain_node_id
         LEFT JOIN (
             SELECT COALESCE(entry.systemtask_id, routine.systemtask_id) AS task_id, COUNT(*) AS total
             FROM calendar_entries entry
             LEFT JOIN systemtask_routine_executions execution ON execution.id = entry.routine_execution_id
             LEFT JOIN systemtask_routines routine ON routine.id = execution.routine_id
             GROUP BY COALESCE(entry.systemtask_id, routine.systemtask_id)
         ) planning ON planning.task_id = task.entity_id
         ORDER BY task.entity_id DESC",
    )
    .fetch_all(pool)
    .await?;
    let entity_ids = tasks.iter().map(|task| task.id).collect::<Vec<_>>();
    let comments = comment_repository::get_comments_for_entities(pool, &entity_ids).await?;
    for task in &mut tasks {
        task.comments = comments
            .iter()
            .filter(|comment| comment.entity_id == task.id)
            .cloned()
            .collect();
    }
    Ok(tasks)
}

pub async fn create_task(
    pool: &PgPool,
    name: &str,
    context_ids: &[i32],
    priority_id: i32,
    domain_node_id: Option<i32>,
    due_at: Option<DateTime<Utc>>,
) -> Result<i32, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let entity_id =
        entity_repository::create_entity(&mut transaction, EntityType::SystemTask).await?;
    sqlx::query(
        "INSERT INTO systemtasks
            (entity_id, name, state_id, priority_id, domain_node_id, due_at)
         VALUES ($1, $2, $3, $4, $5, $6)",
    )
    .bind(entity_id)
    .bind(name)
    .bind(OPEN_SYSTEM_TASK_STATE_ID)
    .bind(priority_id)
    .bind(domain_node_id)
    .bind(due_at)
    .execute(&mut *transaction)
    .await?;
    insert_task_contexts(&mut transaction, entity_id, context_ids).await?;
    transaction.commit().await?;
    Ok(entity_id)
}

async fn insert_task_contexts(
    connection: &mut PgConnection,
    id: i32,
    context_ids: &[i32],
) -> Result<(), sqlx::Error> {
    sqlx::query(
        "INSERT INTO context_entities (context_id, entity_id)
         SELECT DISTINCT context_id, $1 FROM unnest($2::integer[]) AS selection(context_id)",
    )
    .bind(id)
    .bind(context_ids)
    .execute(connection)
    .await?;
    Ok(())
}

pub async fn update_task_contexts(
    pool: &PgPool,
    id: i32,
    context_ids: &[i32],
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let task = sqlx::query_scalar::<_, i32>(
        "SELECT entity_id FROM systemtasks
         WHERE entity_id = $1 AND domain_node_id IS NULL
         FOR UPDATE",
    )
    .bind(id)
    .fetch_optional(&mut *transaction)
    .await?;
    if task.is_none() {
        return Ok(false);
    }

    sqlx::query("DELETE FROM context_entities WHERE entity_id = $1")
        .bind(id)
        .execute(&mut *transaction)
        .await?;
    insert_task_contexts(&mut transaction, id, context_ids).await?;
    transaction.commit().await?;
    Ok(true)
}

pub async fn update_task_field(
    pool: &PgPool,
    id: i32,
    update: TaskFieldUpdate,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;

    let (field, old_value, new_value) = match update {
        TaskFieldUpdate::Name(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, String>(
                "SELECT name FROM systemtasks WHERE entity_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE systemtasks SET name = $2 WHERE entity_id = $1")
                .bind(id)
                .bind(&value)
                .execute(&mut *transaction)
                .await?;
            (TaskField::Name, json!(old_value), json!(value))
        }
        TaskFieldUpdate::StateId(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, i32>(
                "SELECT state_id FROM systemtasks WHERE entity_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE systemtasks SET state_id = $2 WHERE entity_id = $1")
                .bind(id)
                .bind(value)
                .execute(&mut *transaction)
                .await?;
            (TaskField::StateId, json!(old_value), json!(value))
        }
        TaskFieldUpdate::PriorityId(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, i32>(
                "SELECT priority_id FROM systemtasks WHERE entity_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE systemtasks SET priority_id = $2 WHERE entity_id = $1")
                .bind(id)
                .bind(value)
                .execute(&mut *transaction)
                .await?;
            (TaskField::PriorityId, json!(old_value), json!(value))
        }
        TaskFieldUpdate::DueAt(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, Option<DateTime<Utc>>>(
                "SELECT due_at FROM systemtasks WHERE entity_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE systemtasks SET due_at = $2 WHERE entity_id = $1")
                .bind(id)
                .bind(value)
                .execute(&mut *transaction)
                .await?;
            (TaskField::DueAt, json!(old_value), json!(value))
        }
    };

    entity_repository::create_field_version(
        &mut transaction,
        id,
        EntityType::SystemTask,
        field as i32,
        old_value,
        new_value,
    )
    .await?;
    transaction.commit().await?;

    Ok(true)
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
