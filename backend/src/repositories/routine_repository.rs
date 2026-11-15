use crate::models::routine::{DomainNodeRoutine, SaveDomainNodeRoutineRequest};
use sqlx::PgPool;

pub enum CreateRoutineResult {
    Created(DomainNodeRoutine),
    NodeNotFound,
    RoutineExists,
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
         VALUES ($1, $2, 1, 2, $3)
         RETURNING id",
    )
    .bind(domain_node_id)
    .bind(routine.name.trim())
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
