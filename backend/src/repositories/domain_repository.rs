use crate::models::domain::{Domain, DomainNode, DomainNodeField, DomainNodeFieldUpdate};
use crate::models::entity::EntityType;
use crate::repositories::{comment_repository, entity_repository};
use serde_json::json;
use sqlx::PgPool;

pub async fn get_domains(pool: &PgPool) -> Result<Vec<Domain>, sqlx::Error> {
    sqlx::query_as::<_, Domain>("SELECT id, name, color, icon FROM domains ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn update_domain(
    pool: &PgPool,
    id: i32,
    color: Option<&str>,
    icon: Option<&[u8]>,
) -> Result<Option<Domain>, sqlx::Error> {
    sqlx::query_as::<_, Domain>(
        "UPDATE domains
         SET color = $2, icon = $3
         WHERE id = $1
         RETURNING id, name, color, icon",
    )
    .bind(id)
    .bind(color)
    .bind(icon)
    .fetch_optional(pool)
    .await
}

pub async fn get_domain_nodes(pool: &PgPool) -> Result<Vec<DomainNode>, sqlx::Error> {
    let mut nodes = sqlx::query_as::<_, DomainNode>(
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
         SELECT dn.entity_id AS id, dn.domain_id, dn.parent_id, dn.knowledge_level_id, dn.name,
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
    .await?;
    let entity_ids = nodes.iter().map(|node| node.id).collect::<Vec<_>>();
    let comments = comment_repository::get_comments_for_entities(pool, &entity_ids).await?;
    for node in &mut nodes {
        node.comments = comments
            .iter()
            .filter(|comment| comment.entity_id == node.id)
            .cloned()
            .collect();
    }
    Ok(nodes)
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
         RETURNING entity_id AS id, domain_id, parent_id, knowledge_level_id, name, is_system_value,
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
         RETURNING entity_id AS id, domain_id, parent_id, knowledge_level_id, name, is_system_value,
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

pub async fn update_domain_node_field(
    pool: &PgPool,
    id: i32,
    update: DomainNodeFieldUpdate,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;

    let (field, old_value, new_value) = match update {
        DomainNodeFieldUpdate::Name(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, String>(
                "SELECT name FROM domain_nodes WHERE entity_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE domain_nodes SET name = $2 WHERE entity_id = $1")
                .bind(id)
                .bind(&value)
                .execute(&mut *transaction)
                .await?;
            (DomainNodeField::Name, json!(old_value), json!(value))
        }
        DomainNodeFieldUpdate::KnowledgeLevelId(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, Option<i32>>(
                "SELECT knowledge_level_id FROM domain_nodes WHERE entity_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE domain_nodes SET knowledge_level_id = $2 WHERE entity_id = $1")
                .bind(id)
                .bind(value)
                .execute(&mut *transaction)
                .await?;
            (
                DomainNodeField::KnowledgeLevelId,
                json!(old_value),
                json!(value),
            )
        }
        DomainNodeFieldUpdate::CompletenessLevelId(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, Option<i32>>(
                "SELECT completeness_level_id FROM documents WHERE domain_node_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query(
                "UPDATE documents SET completeness_level_id = $2 WHERE domain_node_id = $1",
            )
            .bind(id)
            .bind(value)
            .execute(&mut *transaction)
            .await?;
            (
                DomainNodeField::CompletenessLevelId,
                json!(old_value),
                json!(value),
            )
        }
        DomainNodeFieldUpdate::FlashcardStatusId(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, Option<i32>>(
                "SELECT flashcard_status_id FROM documents WHERE domain_node_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE documents SET flashcard_status_id = $2 WHERE domain_node_id = $1")
                .bind(id)
                .bind(value)
                .execute(&mut *transaction)
                .await?;
            (
                DomainNodeField::FlashcardStatusId,
                json!(old_value),
                json!(value),
            )
        }
        DomainNodeFieldUpdate::VerificationStatusId(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, Option<i32>>(
                "SELECT verification_status_id FROM documents WHERE domain_node_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query(
                "UPDATE documents SET verification_status_id = $2 WHERE domain_node_id = $1",
            )
            .bind(id)
            .bind(value)
            .execute(&mut *transaction)
            .await?;
            (
                DomainNodeField::VerificationStatusId,
                json!(old_value),
                json!(value),
            )
        }
        DomainNodeFieldUpdate::VerificationLevelId(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, Option<i32>>(
                "SELECT verification_level_id FROM documents WHERE domain_node_id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query(
                "UPDATE documents SET verification_level_id = $2 WHERE domain_node_id = $1",
            )
            .bind(id)
            .bind(value)
            .execute(&mut *transaction)
            .await?;
            (
                DomainNodeField::VerificationLevelId,
                json!(old_value),
                json!(value),
            )
        }
    };

    entity_repository::create_field_version(
        &mut transaction,
        id,
        EntityType::DomainNode,
        field as i32,
        old_value,
        new_value,
    )
    .await?;
    transaction.commit().await?;

    Ok(true)
}
