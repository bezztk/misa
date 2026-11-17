use crate::models::calendar_entry::{CalendarEntry, SaveCalendarEntryRequest};
use sqlx::PgPool;

pub async fn get_calendar_entries(pool: &PgPool) -> Result<Vec<CalendarEntry>, sqlx::Error> {
    sqlx::query_as::<_, CalendarEntry>(
        "SELECT id, systemtask_id, routine_execution_id, name, note,
                start_at, end_at, is_locked
         FROM calendar_entries
         ORDER BY start_at, id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_calendar_entry(
    pool: &PgPool,
    entry: &SaveCalendarEntryRequest,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO calendar_entries (systemtask_id, name, start_at, end_at, is_locked)
         VALUES ($1, $2, $3, $4, false)
         RETURNING id",
    )
    .bind(entry.systemtask_id)
    .bind(&entry.name)
    .bind(entry.start_at)
    .bind(entry.end_at)
    .fetch_one(pool)
    .await
}

pub async fn update_calendar_entry(
    pool: &PgPool,
    id: i32,
    entry: &SaveCalendarEntryRequest,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE calendar_entries
         SET systemtask_id = $1, name = $2, start_at = $3, end_at = $4
         WHERE id = $5",
    )
    .bind(entry.systemtask_id)
    .bind(&entry.name)
    .bind(entry.start_at)
    .bind(entry.end_at)
    .bind(id)
    .execute(pool)
    .await?;

    Ok(result.rows_affected() > 0)
}

pub async fn delete_calendar_entry(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM calendar_entries WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
