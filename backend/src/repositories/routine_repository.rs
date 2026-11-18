use crate::models::entity::EntityType;
use chrono::{DateTime, NaiveDate, NaiveDateTime, NaiveTime, Utc};
use sqlx::PgPool;

const PENDING_ROUTINE_EXECUTION_STATE_ID: i32 = 1;

#[derive(sqlx::FromRow)]
pub struct RoutineExecutionSource {
    pub id: i32,
    pub name: String,
    pub recurrence_rule_set: String,
    pub is_optional: bool,
    pub estimated_duration: Option<i32>,
    pub local_now: NaiveDateTime,
    pub has_future_execution: bool,
}

pub struct NewRoutineExecution {
    pub routine_id: i32,
    pub name: String,
    pub is_optional: bool,
    pub estimated_duration: Option<i32>,
    pub occurrence_on: NaiveDate,
    pub time: Option<NaiveTime>,
    pub until_utc: Option<DateTime<Utc>>,
}

pub async fn get_active_routine_execution_sources(
    pool: &PgPool,
) -> Result<Vec<RoutineExecutionSource>, sqlx::Error> {
    sqlx::query_as::<_, RoutineExecutionSource>(
        "SELECT r.id, st.name, r.recurrence_rule_set, r.is_optional,
                st.estimated_effort_minutes AS estimated_duration,
                clock.local_now,
                EXISTS (
                    SELECT 1
                    FROM systemtask_routine_executions execution
                    WHERE execution.routine_id = r.id
                      AND (
                          execution.scheduled_at > clock.now
                          OR (
                              execution.scheduled_at IS NULL
                              AND execution.occurrence_on > clock.local_now::date
                          )
                      )
                ) AS has_future_execution
         FROM systemtask_routines r
         JOIN systemtasks st ON st.entity_id = r.systemtask_id
         CROSS JOIN (
             SELECT NOW() AS now,
                    NOW() AT TIME ZONE 'Europe/Berlin' AS local_now
         ) clock
         WHERE r.is_active",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_routine_executions(
    pool: &PgPool,
    executions: &[NewRoutineExecution],
) -> Result<usize, sqlx::Error> {
    if executions.is_empty() {
        return Ok(0);
    }

    let mut transaction = pool.begin().await?;
    let mut created = 0;
    for execution in executions {
        let result = sqlx::query(
            "WITH candidate AS (
             SELECT $1::INTEGER AS routine_id,
                    $2::TEXT AS name,
                    $3::BOOLEAN AS is_optional,
                    $4::INTEGER AS estimated_duration,
                    $5::DATE AS occurrence_on,
                    $6::TIME AS occurrence_time,
                    $7::TIMESTAMPTZ AS until_utc,
                    CASE
                        WHEN $6::TIME IS NULL
                            THEN $5::DATE::timestamp AT TIME ZONE 'Europe/Berlin'
                        ELSE ($5::DATE + $6::TIME)
                             AT TIME ZONE 'Europe/Berlin'
                    END AS start_at,
                    CASE
                        WHEN $6::TIME IS NULL THEN NULL
                        ELSE ($5::DATE + $6::TIME)
                             AT TIME ZONE 'Europe/Berlin'
                    END AS scheduled_at
         ),
         new_execution AS (
             INSERT INTO systemtask_routine_executions
                 (routine_id, state_id, is_optional, occurrence_on, scheduled_at)
             SELECT routine_id, $8, is_optional, occurrence_on, scheduled_at
             FROM candidate
             WHERE scheduled_at IS NULL
                OR until_utc::timestamptz IS NULL
                OR scheduled_at <= until_utc::timestamptz
             ON CONFLICT (routine_id, occurrence_on) DO NOTHING
             RETURNING id, routine_id, occurrence_on
         ),
         new_entity AS (
             INSERT INTO entities (type_id)
             SELECT $9::INTEGER FROM new_execution
             RETURNING id
         )
         INSERT INTO calendar_entries
             (entity_id, routine_execution_id, name, start_at, end_at, is_locked)
         SELECT new_entity.id,
                new_execution.id,
                candidate.name,
                candidate.start_at,
                CASE
                    WHEN candidate.estimated_duration IS NULL THEN NULL
                    ELSE candidate.start_at
                         + candidate.estimated_duration::integer * INTERVAL '1 minute'
                END,
                false
         FROM new_execution
         JOIN candidate USING (routine_id, occurrence_on)
         CROSS JOIN new_entity",
        )
        .bind(execution.routine_id)
        .bind(&execution.name)
        .bind(execution.is_optional)
        .bind(execution.estimated_duration)
        .bind(execution.occurrence_on)
        .bind(execution.time)
        .bind(execution.until_utc)
        .bind(PENDING_ROUTINE_EXECUTION_STATE_ID)
        .bind(EntityType::CalendarEntry as i32)
        .execute(&mut *transaction)
        .await?;
        created += result.rows_affected() as usize;
    }
    transaction.commit().await?;
    Ok(created)
}
