use crate::models::calendar_entry::{CalendarEntry, SaveCalendarEntryRequest};
use crate::models::entity::EntityType;
use crate::repositories::entity_repository;
use sqlx::PgPool;

pub async fn get_calendar_entries(pool: &PgPool) -> Result<Vec<CalendarEntry>, sqlx::Error> {
    sqlx::query_as::<_, CalendarEntry>(
        "SELECT entry.entity_id AS id, entry.systemtask_id, entry.routine_execution_id,
                routine.systemtask_id AS routine_systemtask_id, assignment.context_id,
                entry.name, entry.note, entry.start_at, entry.end_at, entry.is_locked
         FROM calendar_entries entry
         LEFT JOIN systemtask_routine_executions execution ON execution.id = entry.routine_execution_id
         LEFT JOIN systemtask_routines routine ON routine.id = execution.routine_id
         LEFT JOIN context_entities assignment ON assignment.entity_id = entry.entity_id
         ORDER BY entry.start_at, entry.entity_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_calendar_entry(
    pool: &PgPool,
    entry: &SaveCalendarEntryRequest,
) -> Result<i32, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let entity_id =
        entity_repository::create_entity(&mut transaction, EntityType::CalendarEntry).await?;
    sqlx::query(
        "INSERT INTO calendar_entries
            (entity_id, systemtask_id, name, start_at, end_at, is_locked)
         VALUES ($1, $2, $3, $4, $5, false)",
    )
    .bind(entity_id)
    .bind(entry.systemtask_id)
    .bind(&entry.name)
    .bind(entry.start_at)
    .bind(entry.end_at)
    .execute(&mut *transaction)
    .await?;
    transaction.commit().await?;
    Ok(entity_id)
}

pub async fn update_calendar_entry(
    pool: &PgPool,
    id: i32,
    entry: &SaveCalendarEntryRequest,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE calendar_entries
         SET systemtask_id = $1, name = $2, start_at = $3, end_at = $4, is_locked = $5
         WHERE entity_id = $6",
    )
    .bind(entry.systemtask_id)
    .bind(&entry.name)
    .bind(entry.start_at)
    .bind(entry.end_at)
    .bind(entry.is_locked)
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

pub async fn delete_calendar_entry(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM entities WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
