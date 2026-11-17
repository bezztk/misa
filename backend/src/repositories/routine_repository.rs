use crate::models::routine::{DomainNodeRoutine, SaveDomainNodeRoutineRequest};
use crate::models::system_task::{NORMAL_SYSTEM_TASK_PRIORITY_ID, OPEN_SYSTEM_TASK_STATE_ID};
use chrono::{DateTime, NaiveDate, NaiveDateTime, NaiveTime, Utc};
use sqlx::{PgPool, Postgres, QueryBuilder};

const PENDING_ROUTINE_EXECUTION_STATE_ID: i32 = 1;

pub enum CreateRoutineResult {
    Created(DomainNodeRoutine),
    NodeNotFound,
    RoutineExists,
}

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

pub async fn get_domain_node_routine(
    pool: &PgPool,
    domain_node_id: i32,
) -> Result<Option<DomainNodeRoutine>, sqlx::Error> {
    sqlx::query_as::<_, DomainNodeRoutine>(
        "SELECT r.id, r.systemtask_id, st.domain_node_id, st.name,
                r.recurrence_rule_set, r.timezone,
                st.estimated_effort_minutes AS estimated_duration,
                r.is_optional, r.is_active
         FROM systemtask_routines r
         JOIN systemtasks st ON st.id = r.systemtask_id
         WHERE st.domain_node_id = $1",
    )
    .bind(domain_node_id)
    .fetch_optional(pool)
    .await
}

pub async fn create_domain_node_routine(
    pool: &PgPool,
    domain_node_id: i32,
    routine: &SaveDomainNodeRoutineRequest,
) -> Result<CreateRoutineResult, sqlx::Error> {
    let mut transaction = pool.begin().await?;

    sqlx::query("SELECT pg_advisory_xact_lock(7281, $1)")
        .bind(domain_node_id)
        .execute(&mut *transaction)
        .await?;

    let node_exists =
        sqlx::query_scalar::<_, bool>("SELECT EXISTS (SELECT 1 FROM domain_nodes WHERE id = $1)")
            .bind(domain_node_id)
            .fetch_one(&mut *transaction)
            .await?;
    if !node_exists {
        return Ok(CreateRoutineResult::NodeNotFound);
    }

    let routine_exists = sqlx::query_scalar::<_, bool>(
        "SELECT EXISTS (
             SELECT 1
             FROM systemtask_routines r
             JOIN systemtasks st ON st.id = r.systemtask_id
             WHERE st.domain_node_id = $1
         )",
    )
    .bind(domain_node_id)
    .fetch_one(&mut *transaction)
    .await?;
    if routine_exists {
        return Ok(CreateRoutineResult::RoutineExists);
    }

    let systemtask_id = sqlx::query_scalar::<_, i32>(
        "INSERT INTO systemtasks
             (domain_node_id, name, state_id, priority_id, estimated_effort_minutes)
         VALUES ($1, $2, $3, $4, $5)
         RETURNING id",
    )
    .bind(domain_node_id)
    .bind(routine.name.trim())
    .bind(OPEN_SYSTEM_TASK_STATE_ID)
    .bind(NORMAL_SYSTEM_TASK_PRIORITY_ID)
    .bind(routine.estimated_duration)
    .fetch_one(&mut *transaction)
    .await?;

    let id = sqlx::query_scalar::<_, i32>(
        "INSERT INTO systemtask_routines
             (systemtask_id, recurrence_rule_set, timezone, is_optional, is_active)
         VALUES ($1, $2, $3, $4, $5)
         RETURNING id",
    )
    .bind(systemtask_id)
    .bind(&routine.recurrence_rule_set)
    .bind(&routine.timezone)
    .bind(routine.is_optional)
    .bind(routine.is_active)
    .fetch_one(&mut *transaction)
    .await?;

    transaction.commit().await?;

    Ok(CreateRoutineResult::Created(DomainNodeRoutine {
        id,
        systemtask_id,
        domain_node_id,
        name: routine.name.trim().to_string(),
        recurrence_rule_set: routine.recurrence_rule_set.clone(),
        timezone: routine.timezone.clone(),
        estimated_duration: routine.estimated_duration,
        is_optional: routine.is_optional,
        is_active: routine.is_active,
    }))
}

pub async fn update_domain_node_routine(
    pool: &PgPool,
    domain_node_id: i32,
    routine: &SaveDomainNodeRoutineRequest,
) -> Result<Option<DomainNodeRoutine>, sqlx::Error> {
    let mut transaction = pool.begin().await?;

    sqlx::query("SELECT pg_advisory_xact_lock(7281, $1)")
        .bind(domain_node_id)
        .execute(&mut *transaction)
        .await?;

    let ids = sqlx::query_as::<_, (i32, i32)>(
        "SELECT r.id, r.systemtask_id
         FROM systemtask_routines r
         JOIN systemtasks st ON st.id = r.systemtask_id
         WHERE st.domain_node_id = $1
         FOR UPDATE OF r, st",
    )
    .bind(domain_node_id)
    .fetch_optional(&mut *transaction)
    .await?;

    let Some((id, systemtask_id)) = ids else {
        return Ok(None);
    };

    sqlx::query(
        "UPDATE systemtasks
         SET name = $1, estimated_effort_minutes = $2
         WHERE id = $3",
    )
    .bind(routine.name.trim())
    .bind(routine.estimated_duration)
    .bind(systemtask_id)
    .execute(&mut *transaction)
    .await?;

    sqlx::query(
        "UPDATE systemtask_routines
         SET recurrence_rule_set = $1, timezone = $2,
             is_optional = $3, is_active = $4
         WHERE id = $5",
    )
    .bind(&routine.recurrence_rule_set)
    .bind(&routine.timezone)
    .bind(routine.is_optional)
    .bind(routine.is_active)
    .bind(id)
    .execute(&mut *transaction)
    .await?;

    transaction.commit().await?;

    Ok(Some(DomainNodeRoutine {
        id,
        systemtask_id,
        domain_node_id,
        name: routine.name.trim().to_string(),
        recurrence_rule_set: routine.recurrence_rule_set.clone(),
        timezone: routine.timezone.clone(),
        estimated_duration: routine.estimated_duration,
        is_optional: routine.is_optional,
        is_active: routine.is_active,
    }))
}
