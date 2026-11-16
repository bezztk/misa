use crate::models::domain::{Domain, DomainNode};
use sqlx::PgPool;

pub async fn get_domains(pool: &PgPool) -> Result<Vec<Domain>, sqlx::Error> {
    sqlx::query_as::<_, Domain>("SELECT id, name FROM domains ORDER BY id")
        .fetch_all(pool)
        .await
}

pub async fn get_domain_nodes(pool: &PgPool) -> Result<Vec<DomainNode>, sqlx::Error> {
    sqlx::query_as::<_, DomainNode>(
        "SELECT dn.id, dn.domain_id, dn.parent_id, dn.cognitive_level_id, dn.name,
                dn.is_system_value, dn.last_reviewed_at,
                EXISTS (SELECT 1 FROM documents WHERE domain_node_id = dn.id) AS has_document,
                EXISTS (
                    SELECT 1 FROM systemtask_routines r
                    JOIN systemtasks st ON st.id = r.systemtask_id
                    WHERE st.domain_node_id = dn.id
                ) AS has_routine,
                COUNT(f.id) AS flashcard_count,
                (SELECT COUNT(*) FROM skills WHERE domain_node_id = dn.id) AS skill_count,
                (SELECT COUNT(*) FROM pursuits WHERE domain_node_id = dn.id) AS pursuit_count,
                (SELECT COUNT(*)
                 FROM systemtasks st
                 JOIN pursuits p ON p.id = st.pursuit_id
                 WHERE p.domain_node_id = dn.id) AS task_count,
                d.completeness_level_id, d.flashcard_status_id,
                d.verification_status_id, d.verification_level_id
         FROM domain_nodes dn
         LEFT JOIN flashcards f ON f.domain_node_id = dn.id
         LEFT JOIN documents d ON d.domain_node_id = dn.id
         GROUP BY dn.id, d.domain_node_id
         ORDER BY dn.domain_id, dn.id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_domain_node(
    pool: &PgPool,
    parent_id: i32,
    name: &str,
) -> Result<Option<DomainNode>, sqlx::Error> {
    sqlx::query_as::<_, DomainNode>(
        "INSERT INTO domain_nodes (domain_id, parent_id, name, is_system_value)
         SELECT domain_id, id, $2, FALSE
         FROM domain_nodes
         WHERE id = $1
         RETURNING id, domain_id, parent_id, cognitive_level_id, name, is_system_value,
                   last_reviewed_at, FALSE AS has_document, 0::BIGINT AS flashcard_count,
                   FALSE AS has_routine,
                   0::BIGINT AS skill_count,
                   0::BIGINT AS pursuit_count,
                   0::BIGINT AS task_count,
                   NULL::INTEGER AS completeness_level_id,
                   NULL::INTEGER AS flashcard_status_id,
                   NULL::INTEGER AS verification_status_id,
                   NULL::INTEGER AS verification_level_id",
    )
    .bind(parent_id)
    .bind(name)
    .fetch_optional(pool)
    .await
}

pub async fn create_root_domain_node(
    pool: &PgPool,
    domain_id: i32,
    name: &str,
) -> Result<Option<DomainNode>, sqlx::Error> {
    sqlx::query_as::<_, DomainNode>(
        "INSERT INTO domain_nodes (domain_id, parent_id, name, is_system_value)
         SELECT id, NULL, $2, FALSE
         FROM domains
         WHERE id = $1
         RETURNING id, domain_id, parent_id, cognitive_level_id, name, is_system_value,
                   last_reviewed_at, FALSE AS has_document, 0::BIGINT AS flashcard_count,
                   FALSE AS has_routine,
                   0::BIGINT AS skill_count,
                   0::BIGINT AS pursuit_count,
                   0::BIGINT AS task_count,
                   NULL::INTEGER AS completeness_level_id,
                   NULL::INTEGER AS flashcard_status_id,
                   NULL::INTEGER AS verification_status_id,
                   NULL::INTEGER AS verification_level_id",
    )
    .bind(domain_id)
    .bind(name)
    .fetch_optional(pool)
    .await
}

pub async fn update_domain_node(
    pool: &PgPool,
    id: i32,
    name: &str,
    cognitive_level_id: Option<i32>,
    completeness_level_id: Option<i32>,
    flashcard_status_id: Option<i32>,
    verification_status_id: Option<i32>,
    verification_level_id: Option<i32>,
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
    .bind(completeness_level_id)
    .bind(flashcard_status_id)
    .bind(verification_status_id)
    .bind(verification_level_id)
    .execute(&mut *transaction)
    .await?;

    let node = sqlx::query_as::<_, DomainNode>(
        "UPDATE domain_nodes
         SET name = $2, cognitive_level_id = $3
         WHERE id = $1
         RETURNING id, domain_id, parent_id, cognitive_level_id, name, is_system_value, last_reviewed_at,
                   EXISTS (SELECT 1 FROM documents WHERE domain_node_id = domain_nodes.id) AS has_document,
                   EXISTS (
                       SELECT 1 FROM systemtask_routines r
                       JOIN systemtasks st ON st.id = r.systemtask_id
                       WHERE st.domain_node_id = domain_nodes.id
                   ) AS has_routine,
                   (SELECT COUNT(*) FROM flashcards WHERE domain_node_id = domain_nodes.id) AS flashcard_count,
                   (SELECT COUNT(*) FROM skills WHERE domain_node_id = domain_nodes.id) AS skill_count,
                   (SELECT COUNT(*) FROM pursuits WHERE domain_node_id = domain_nodes.id) AS pursuit_count,
                   (SELECT COUNT(*)
                    FROM systemtasks st
                    JOIN pursuits p ON p.id = st.pursuit_id
                    WHERE p.domain_node_id = domain_nodes.id) AS task_count,
                   (SELECT completeness_level_id FROM documents WHERE domain_node_id = domain_nodes.id) AS completeness_level_id,
                   (SELECT flashcard_status_id FROM documents WHERE domain_node_id = domain_nodes.id) AS flashcard_status_id,
                   (SELECT verification_status_id FROM documents WHERE domain_node_id = domain_nodes.id) AS verification_status_id,
                   (SELECT verification_level_id FROM documents WHERE domain_node_id = domain_nodes.id) AS verification_level_id",
    )
    .bind(id)
    .bind(name)
    .bind(cognitive_level_id)
    .fetch_optional(&mut *transaction)
    .await?;

    transaction.commit().await?;
    Ok(node)
}
