use crate::models::calendar_entry::CalendarEntry;
use crate::models::entity::EntityType;
use crate::repositories::entity_repository;
use chrono::{DateTime, Utc};
use sqlx::PgPool;

pub async fn create_calendar_entry(
    pool: &PgPool,
    source_entity_id: Option<i32>,
    name: &str,
    note: Option<&str>,
    is_reminder: bool,
    is_locked: bool,
    estimated_duration: Option<i32>,
    start_at: DateTime<Utc>,
    end_at: Option<DateTime<Utc>>,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let entity_id = entity_repository::create_entity(&mut tx, EntityType::CalendarEntry).await?;

    sqlx::query(
        "INSERT INTO calendar_entries
            (entity_id, source_entity_id, name, note, is_reminder, is_locked,
             estimated_duration, start_at, end_at)
         VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)",
    )
    .bind(entity_id)
    .bind(source_entity_id)
    .bind(name)
    .bind(note)
    .bind(is_reminder)
    .bind(is_locked)
    .bind(estimated_duration)
    .bind(start_at)
    .bind(end_at)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;
    Ok(entity_id)
}

pub async fn get_calendar_entry(
    pool: &PgPool,
    entity_id: i32,
) -> Result<Option<CalendarEntry>, sqlx::Error> {
    sqlx::query_as::<_, CalendarEntry>(
        "SELECT
            c.entity_id,
            e.created_at,
            e.updated_at,
            e.version,
            c.source_entity_id,
            c.name,
            c.note,
            c.is_reminder,
            c.is_locked,
            c.estimated_duration,
            c.start_at,
            c.end_at
         FROM calendar_entries c
         JOIN entities e ON e.id = c.entity_id
         WHERE c.entity_id = $1
           AND NOT e.is_archived
           AND NOT e.is_deleted",
    )
    .bind(entity_id)
    .fetch_optional(pool)
    .await
}

pub async fn get_calendar_entries(pool: &PgPool) -> Result<Vec<CalendarEntry>, sqlx::Error> {
    sqlx::query_as::<_, CalendarEntry>(
        "SELECT
            c.entity_id,
            e.created_at,
            e.updated_at,
            e.version,
            c.source_entity_id,
            c.name,
            c.note,
            c.is_reminder,
            c.is_locked,
            c.estimated_duration,
            c.start_at,
            c.end_at
         FROM calendar_entries c
         JOIN entities e ON e.id = c.entity_id
         WHERE NOT e.is_archived AND NOT e.is_deleted
         ORDER BY c.start_at, c.entity_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn update_calendar_entry(
    pool: &PgPool,
    entity_id: i32,
    name: &str,
    note: Option<&str>,
    is_reminder: bool,
    is_locked: bool,
    estimated_duration: Option<i32>,
    start_at: DateTime<Utc>,
    end_at: Option<DateTime<Utc>>,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE calendar_entries
         SET name = $1,
             note = $2,
             is_reminder = $3,
             is_locked = $4,
             estimated_duration = $5,
             start_at = $6,
             end_at = $7
         WHERE entity_id = $8",
    )
    .bind(name)
    .bind(note)
    .bind(is_reminder)
    .bind(is_locked)
    .bind(estimated_duration)
    .bind(start_at)
    .bind(end_at)
    .bind(entity_id)
    .execute(&mut *tx)
    .await?;

    if result.rows_affected() == 0 {
        return Ok(false);
    }

    entity_repository::update_entity(&mut tx, entity_id).await?;
    tx.commit().await?;
    Ok(true)
}
