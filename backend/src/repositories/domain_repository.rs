use crate::models::domain::{Domain, DomainNode, DomainNodeUpdateRequest};
use crate::models::entity::EntityType;
use crate::repositories::entity_repository;
use sqlx::PgPool;

pub async fn get_domains(pool: &PgPool) -> Result<Vec<Domain>, sqlx::Error> {
    sqlx::query_as::<_, Domain>("SELECT id, name FROM domains ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_domain_nodes(pool: &PgPool) -> Result<Vec<DomainNode>, sqlx::Error> {
    sqlx::query_as::<_, DomainNode>(
        "WITH flashcard_counts AS (
             SELECT domain_node_id, COUNT(*) AS count
             FROM flashcards
             GROUP BY domain_node_id
         ),
         attachment_counts AS (
             SELECT domain_node_id, COUNT(*) AS count
             FROM attachments
             GROUP BY domain_node_id
         ),
         skill_counts AS (
             SELECT domain_node_id, COUNT(*) AS count
             FROM skills
             GROUP BY domain_node_id
         ),
         pursuit_counts AS (
             SELECT domain_node_id, COUNT(*) AS count
             FROM pursuits
             GROUP BY domain_node_id
         ),
         task_counts AS (
             SELECT domain_node_id, COUNT(*) AS count
             FROM systemtasks
             WHERE domain_node_id IS NOT NULL
             GROUP BY domain_node_id
         )
         SELECT dn.entity_id AS id, dn.domain_id, dn.parent_id, dn.cognitive_level_id, dn.name,
                dn.is_system_value, dn.last_reviewed_at,
                d.domain_node_id IS NOT NULL AS has_document,
                COALESCE(fc.count, 0) AS flashcard_count,
                COALESCE(ac.count, 0) AS attachment_count,
                COALESCE(sc.count, 0) AS skill_count,
                COALESCE(pc.count, 0) AS pursuit_count,
                COALESCE(tc.count, 0) AS task_count,
                d.completeness_level_id, d.flashcard_status_id,
                d.verification_status_id, d.verification_level_id
         FROM domain_nodes dn
         LEFT JOIN documents d ON d.domain_node_id = dn.entity_id
         LEFT JOIN flashcard_counts fc ON fc.domain_node_id = dn.entity_id
         LEFT JOIN attachment_counts ac ON ac.domain_node_id = dn.entity_id
         LEFT JOIN skill_counts sc ON sc.domain_node_id = dn.entity_id
         LEFT JOIN pursuit_counts pc ON pc.domain_node_id = dn.entity_id
         LEFT JOIN task_counts tc ON tc.domain_node_id = dn.entity_id
         ORDER BY dn.domain_id, dn.entity_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_domain_node(
    pool: &PgPool,
    parent_id: i32,
    name: &str,
) -> Result<Option<DomainNode>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let domain_id =
        sqlx::query_scalar::<_, i32>("SELECT domain_id FROM domain_nodes WHERE entity_id = $1")
            .bind(parent_id)
            .fetch_optional(&mut *transaction)
            .await?;
    let Some(domain_id) = domain_id else {
        return Ok(None);
    };
    let entity_id =
        entity_repository::create_entity(&mut transaction, EntityType::DomainNode).await?;
    let node = sqlx::query_as::<_, DomainNode>(
        "INSERT INTO domain_nodes (entity_id, domain_id, parent_id, name, is_system_value)
         VALUES ($1, $2, $3, $4, FALSE)
         RETURNING entity_id AS id, domain_id, parent_id, cognitive_level_id, name, is_system_value,
                   last_reviewed_at, FALSE AS has_document, 0::BIGINT AS flashcard_count,
                   0::BIGINT AS attachment_count,
                   0::BIGINT AS skill_count,
                   0::BIGINT AS pursuit_count,
                   0::BIGINT AS task_count,
                   NULL::INTEGER AS completeness_level_id,
                   NULL::INTEGER AS flashcard_status_id,
                   NULL::INTEGER AS verification_status_id,
                   NULL::INTEGER AS verification_level_id",
    )
    .bind(entity_id)
    .bind(domain_id)
    .bind(parent_id)
    .bind(name)
    .fetch_one(&mut *transaction)
    .await?;
    transaction.commit().await?;
    Ok(Some(node))
}

pub async fn create_root_domain_node(
    pool: &PgPool,
    domain_id: i32,
    name: &str,
) -> Result<Option<DomainNode>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let domain_exists =
        sqlx::query_scalar::<_, bool>("SELECT EXISTS (SELECT 1 FROM domains WHERE id = $1)")
            .bind(domain_id)
            .fetch_one(&mut *transaction)
            .await?;
    if !domain_exists {
        return Ok(None);
    }
    let entity_id =
        entity_repository::create_entity(&mut transaction, EntityType::DomainNode).await?;
    let node = sqlx::query_as::<_, DomainNode>(
        "INSERT INTO domain_nodes (entity_id, domain_id, parent_id, name, is_system_value)
         VALUES ($1, $2, NULL, $3, FALSE)
         RETURNING entity_id AS id, domain_id, parent_id, cognitive_level_id, name, is_system_value,
                   last_reviewed_at, FALSE AS has_document, 0::BIGINT AS flashcard_count,
                   0::BIGINT AS attachment_count,
                   0::BIGINT AS skill_count,
                   0::BIGINT AS pursuit_count,
                   0::BIGINT AS task_count,
                   NULL::INTEGER AS completeness_level_id,
                   NULL::INTEGER AS flashcard_status_id,
                   NULL::INTEGER AS verification_status_id,
                   NULL::INTEGER AS verification_level_id",
    )
    .bind(entity_id)
    .bind(domain_id)
    .bind(name)
    .fetch_one(&mut *transaction)
    .await?;
    transaction.commit().await?;
    Ok(Some(node))
}

pub async fn delete_domain_node(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    sqlx::query(
        "WITH RECURSIVE node_tree (entity_id) AS (
             SELECT entity_id FROM domain_nodes WHERE entity_id = $1
             UNION ALL
             SELECT node.entity_id
             FROM domain_nodes node
             JOIN node_tree parent ON parent.entity_id = node.parent_id
         ),
         task_entities (entity_id) AS (
             SELECT task.entity_id
             FROM systemtasks task
             JOIN node_tree node ON node.entity_id = task.domain_node_id
         )
         DELETE FROM systemtask_sessions
         WHERE systemtask_id IN (SELECT entity_id FROM task_entities)
            OR routine_execution_id IN (
                SELECT execution.id
                FROM systemtask_routine_executions execution
                JOIN systemtask_routines routine ON routine.id = execution.routine_id
                WHERE routine.systemtask_id IN (SELECT entity_id FROM task_entities)
            )",
    )
    .bind(id)
    .execute(&mut *transaction)
    .await?;

    let result = sqlx::query(
        "WITH RECURSIVE node_tree (entity_id) AS (
             SELECT entity_id FROM domain_nodes WHERE entity_id = $1
             UNION ALL
             SELECT node.entity_id
             FROM domain_nodes node
             JOIN node_tree parent ON parent.entity_id = node.parent_id
         ),
         task_entities (entity_id) AS (
             SELECT task.entity_id
             FROM systemtasks task
             JOIN node_tree node ON node.entity_id = task.domain_node_id
         ),
         related_entities (entity_id) AS (
             SELECT entity_id FROM node_tree
             UNION
             SELECT attachment.entity_id
             FROM attachments attachment
             JOIN node_tree node ON node.entity_id = attachment.domain_node_id
             UNION
             SELECT flashcard.entity_id
             FROM flashcards flashcard
             JOIN node_tree node ON node.entity_id = flashcard.domain_node_id
             UNION
             SELECT skill.entity_id
             FROM skills skill
             JOIN node_tree node ON node.entity_id = skill.domain_node_id
             UNION
             SELECT pursuit.entity_id
             FROM pursuits pursuit
             JOIN node_tree node ON node.entity_id = pursuit.domain_node_id
             UNION
             SELECT entity_id FROM task_entities
             UNION
             SELECT entry.entity_id
             FROM calendar_entries entry
             WHERE entry.systemtask_id IN (SELECT entity_id FROM task_entities)
                OR entry.routine_execution_id IN (
                    SELECT execution.id
                    FROM systemtask_routine_executions execution
                    JOIN systemtask_routines routine ON routine.id = execution.routine_id
                    WHERE routine.systemtask_id IN (SELECT entity_id FROM task_entities)
                )
         )
         DELETE FROM entities
         WHERE id IN (SELECT entity_id FROM related_entities)",
    )
    .bind(id)
    .execute(&mut *transaction)
    .await?;

    transaction.commit().await?;

    Ok(result.rows_affected() > 0)
}

pub async fn update_domain_node(
    pool: &PgPool,
    id: i32,
    update: &DomainNodeUpdateRequest,
) -> Result<Option<DomainNode>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    sqlx::query(
        "UPDATE documents
         SET completeness_level_id = $2,
             flashcard_status_id = $3,
             verification_status_id = $4,
             verification_level_id = $5
         WHERE domain_node_id = $1",
    )
    .bind(id)
    .bind(update.completeness_level_id)
    .bind(update.flashcard_status_id)
    .bind(update.verification_status_id)
    .bind(update.verification_level_id)
    .execute(&mut *transaction)
    .await?;

    let node = sqlx::query_as::<_, DomainNode>(
        "UPDATE domain_nodes
         SET name = $2, cognitive_level_id = $3
         WHERE entity_id = $1
         RETURNING entity_id AS id, domain_id, parent_id, cognitive_level_id, name, is_system_value, last_reviewed_at,
                   EXISTS (SELECT 1 FROM documents WHERE domain_node_id = domain_nodes.entity_id) AS has_document,
                   (SELECT COUNT(*) FROM flashcards WHERE domain_node_id = domain_nodes.entity_id) AS flashcard_count,
                   (SELECT COUNT(*) FROM attachments WHERE domain_node_id = domain_nodes.entity_id) AS attachment_count,
                   (SELECT COUNT(*) FROM skills WHERE domain_node_id = domain_nodes.entity_id) AS skill_count,
                   (SELECT COUNT(*) FROM pursuits WHERE domain_node_id = domain_nodes.entity_id) AS pursuit_count,
                   (SELECT COUNT(*)
                    FROM systemtasks
                    WHERE domain_node_id = domain_nodes.entity_id) AS task_count,
                   (SELECT completeness_level_id FROM documents WHERE domain_node_id = domain_nodes.entity_id) AS completeness_level_id,
                   (SELECT flashcard_status_id FROM documents WHERE domain_node_id = domain_nodes.entity_id) AS flashcard_status_id,
                   (SELECT verification_status_id FROM documents WHERE domain_node_id = domain_nodes.entity_id) AS verification_status_id,
                   (SELECT verification_level_id FROM documents WHERE domain_node_id = domain_nodes.entity_id) AS verification_level_id",
    )
    .bind(id)
    .bind(&update.name)
    .bind(update.cognitive_level_id)
    .fetch_optional(&mut *transaction)
    .await?;

    if node.is_some() {
        sqlx::query("UPDATE entities SET version = version + 1, updated_at = NOW() WHERE id = $1")
            .bind(id)
            .execute(&mut *transaction)
            .await?;
    }

    transaction.commit().await?;
    Ok(node)
}
