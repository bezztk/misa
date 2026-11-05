use crate::models::entity::EntityType;
use crate::models::task::Task;
use crate::models::task_metadata::{
    TaskGroup, TaskMetadata, TaskPriority, TaskState, TaskStateRow,
};
use crate::repositories::entity_repository;
use chrono::{DateTime, Utc};
use sqlx::PgPool;

pub async fn create_task(
    pool: &PgPool,
    group_id: i32,
    name: &str,
    description: Option<&str>,
    priority_id: i32,
    due_at: Option<DateTime<Utc>>,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let entity_id = entity_repository::create_entity(&mut tx, EntityType::Task).await?;

    sqlx::query(
        "INSERT INTO tasks
            (entity_id, group_id, name, description, state_id, priority_id, due_at)
         VALUES ($1, $2, $3, $4, $5, $6, $7)",
    )
    .bind(entity_id)
    .bind(group_id)
    .bind(name)
    .bind(description)
    .bind(TaskState::Open as i32)
    .bind(priority_id)
    .bind(due_at)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;
    Ok(entity_id)
}

pub async fn get_task(pool: &PgPool, entity_id: i32) -> Result<Option<Task>, sqlx::Error> {
    sqlx::query_as::<_, Task>(
        "SELECT
            t.entity_id,
            e.created_at,
            e.updated_at,
            e.version,
            t.group_id,
            t.name,
            t.description,
            t.state_id,
            t.priority_id,
            t.due_at,
            t.completed_at
         FROM tasks t
         JOIN entities e ON e.id = t.entity_id
         WHERE t.entity_id = $1",
    )
    .bind(entity_id)
    .fetch_optional(pool)
    .await
}

pub async fn get_tasks(pool: &PgPool) -> Result<Vec<Task>, sqlx::Error> {
    sqlx::query_as::<_, Task>(
        "SELECT
            t.entity_id,
            e.created_at,
            e.updated_at,
            e.version,
            t.group_id,
            t.name,
            t.description,
            t.state_id,
            t.priority_id,
            t.due_at,
            t.completed_at
         FROM tasks t
         JOIN entities e ON e.id = t.entity_id
         ORDER BY e.created_at DESC, t.entity_id DESC",
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

pub async fn task_priority_exists(pool: &PgPool, priority_id: i32) -> Result<bool, sqlx::Error> {
    sqlx::query_scalar("SELECT EXISTS(SELECT 1 FROM task_priorities WHERE id = $1)")
        .bind(priority_id)
        .fetch_one(pool)
        .await
}

pub async fn update_task(
    pool: &PgPool,
    entity_id: i32,
    group_id: Option<i32>,
    name: Option<&str>,
    description: Option<&str>,
    state_id: Option<i32>,
    priority_id: Option<i32>,
    due_at: Option<DateTime<Utc>>,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE tasks
         SET group_id = COALESCE($1, group_id),
             name = COALESCE($2, name),
             description = COALESCE($3, description),
             completed_at = CASE
                 WHEN $4::integer IS NULL OR $4 = state_id THEN completed_at
                 WHEN $4 = $8 THEN NOW()
                 ELSE NULL
             END,
             state_id = COALESCE($4, state_id),
             priority_id = COALESCE($5, priority_id),
             due_at = COALESCE($6, due_at)
         WHERE entity_id = $7",
    )
    .bind(group_id)
    .bind(name)
    .bind(description)
    .bind(state_id)
    .bind(priority_id)
    .bind(due_at)
    .bind(entity_id)
    .bind(TaskState::Completed as i32)
    .execute(&mut *tx)
    .await?;

    if result.rows_affected() == 0 {
        return Ok(false);
    }

    entity_repository::update_entity(&mut tx, entity_id).await?;
    tx.commit().await?;
    Ok(true)
}

pub async fn delete_task(pool: &PgPool, entity_id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM entities WHERE id = $1 AND type = $2")
        .bind(entity_id)
        .bind(EntityType::Task as i32)
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
    let priorities =
        sqlx::query_as::<_, TaskPriority>("SELECT id, name FROM task_priorities ORDER BY id")
            .fetch_all(pool)
            .await?;

    Ok(TaskMetadata {
        groups,
        states,
        priorities,
    })
}
