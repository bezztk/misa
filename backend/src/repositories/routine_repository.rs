use chrono::{DateTime, NaiveDate, NaiveDateTime, NaiveTime, Utc};
use sqlx::{PgPool, Postgres, QueryBuilder};

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
         JOIN systemtasks st ON st.id = r.systemtask_id
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

    let mut query = QueryBuilder::<Postgres>::new(
        "WITH input (
             routine_id, name, is_optional, estimated_duration,
             occurrence_on, occurrence_time, until_utc
         ) AS (",
    );
    query.push_values(executions, |mut values, execution| {
        values
            .push_bind(execution.routine_id)
            .push_bind(&execution.name)
            .push_bind(execution.is_optional)
            .push_bind(execution.estimated_duration)
            .push_bind(execution.occurrence_on)
            .push_bind(execution.time)
            .push_bind(execution.until_utc);
    });
    query.push(
        ", candidate AS (
             SELECT input.*,
                    CASE
                        WHEN occurrence_time::time IS NULL
                            THEN occurrence_on::date::timestamp AT TIME ZONE 'Europe/Berlin'
                        ELSE (occurrence_on::date + occurrence_time::time)
                             AT TIME ZONE 'Europe/Berlin'
                    END AS start_at,
                    CASE
                        WHEN occurrence_time::time IS NULL THEN NULL
                        ELSE (occurrence_on::date + occurrence_time::time)
                             AT TIME ZONE 'Europe/Berlin'
                    END AS scheduled_at
             FROM input
         ),
         new_execution AS (
             INSERT INTO systemtask_routine_executions
                 (routine_id, state_id, is_optional, occurrence_on, scheduled_at)
             SELECT routine_id, ",
    );
    query.push_bind(PENDING_ROUTINE_EXECUTION_STATE_ID);
    query.push(
        ", is_optional, occurrence_on, scheduled_at
             FROM candidate
             WHERE scheduled_at IS NULL
                OR until_utc::timestamptz IS NULL
                OR scheduled_at <= until_utc::timestamptz
             ON CONFLICT (routine_id, occurrence_on) DO NOTHING
             RETURNING id, routine_id, occurrence_on
         )
         INSERT INTO calendar_entries
             (routine_execution_id, name, start_at, end_at, is_locked)
         SELECT new_execution.id,
                candidate.name,
                candidate.start_at,
                CASE
                    WHEN candidate.estimated_duration IS NULL THEN NULL
                    ELSE candidate.start_at
                         + candidate.estimated_duration::integer * INTERVAL '1 minute'
                END,
                false
         FROM new_execution
         JOIN candidate USING (routine_id, occurrence_on)",
    );

    let result = query.build().execute(pool).await?;
    Ok(result.rows_affected() as usize)
}
