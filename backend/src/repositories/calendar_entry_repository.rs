use crate::models::calendar_entry::CalendarEntry;
use crate::models::entity::EntityType;
use crate::repositories::entity_repository;
use chrono::{DateTime, Utc};
use sqlx::PgPool;

pub async fn create_calendar_entry(
    pool: &PgPool,
    source_entity_id: Option<i32>,
    is_reminder: bool,
    planned_start_at: DateTime<Utc>,
    planned_end_at: Option<DateTime<Utc>>,
    actual_start_at: Option<DateTime<Utc>>,
    actual_end_at: Option<DateTime<Utc>>,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let entity_id = entity_repository::create_entity(&mut tx, EntityType::CalendarEntry).await?;

    sqlx::query(
        "INSERT INTO calendar_entries
            (entity_id, source_entity_id, is_reminder, planned_start_at, planned_end_at,
             actual_start_at, actual_end_at)
         VALUES ($1, $2, $3, $4, $5, $6, $7)",
    )
    .bind(entity_id)
    .bind(source_entity_id)
    .bind(is_reminder)
    .bind(planned_start_at)
    .bind(planned_end_at)
    .bind(actual_start_at)
    .bind(actual_end_at)
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
            c.is_reminder,
            c.planned_start_at,
            c.planned_end_at,
            c.actual_start_at,
            c.actual_end_at
         FROM calendar_entries c
         JOIN entities e ON e.id = c.entity_id
         WHERE c.entity_id = $1",
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
            c.is_reminder,
            c.planned_start_at,
            c.planned_end_at,
            c.actual_start_at,
            c.actual_end_at
         FROM calendar_entries c
         JOIN entities e ON e.id = c.entity_id
         ORDER BY c.planned_start_at, c.entity_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn update_calendar_entry(
    pool: &PgPool,
    entity_id: i32,
    source_entity_id: Option<i32>,
    is_reminder: bool,
    planned_start_at: DateTime<Utc>,
    planned_end_at: Option<DateTime<Utc>>,
    actual_start_at: Option<DateTime<Utc>>,
    actual_end_at: Option<DateTime<Utc>>,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE calendar_entries
         SET source_entity_id = $1,
             is_reminder = $2,
             planned_start_at = $3,
             planned_end_at = $4,
             actual_start_at = $5,
             actual_end_at = $6
         WHERE entity_id = $7",
    )
    .bind(source_entity_id)
    .bind(is_reminder)
    .bind(planned_start_at)
    .bind(planned_end_at)
    .bind(actual_start_at)
    .bind(actual_end_at)
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

pub async fn delete_calendar_entry(pool: &PgPool, entity_id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM entities WHERE id = $1 AND type = $2")
        .bind(entity_id)
        .bind(EntityType::CalendarEntry as i32)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
