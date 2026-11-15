use crate::models::calendar_entry::{CalendarEntry, CreateCalendarEntryRequest};
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
    entry: &CreateCalendarEntryRequest,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO calendar_entries (systemtask_id, name, start_at, end_at, is_locked)
         VALUES ($1, $2, $3, $4, false)
         RETURNING id",
    )
    .bind(entry.systemtask_id)
    .bind(entry.name.trim())
    .bind(entry.start_at)
    .bind(entry.end_at)
    .fetch_one(pool)
    .await
}

pub async fn update_calendar_entry(
    pool: &PgPool,
    id: i32,
    entry: &CreateCalendarEntryRequest,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE calendar_entries
         SET systemtask_id = $1, name = $2, start_at = $3, end_at = $4
         WHERE id = $5",
    )
    .bind(entry.systemtask_id)
    .bind(entry.name.trim())
    .bind(entry.start_at)
    .bind(entry.end_at)
    .bind(id)
    .execute(pool)
    .await?;

    Ok(result.rows_affected() > 0)
}
