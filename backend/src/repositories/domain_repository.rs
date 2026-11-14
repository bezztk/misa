use crate::models::domain::DomainNode;
use sqlx::PgPool;

pub async fn get_domain_nodes(pool: &PgPool) -> Result<Vec<DomainNode>, sqlx::Error> {
    sqlx::query_as::<_, DomainNode>(
        "SELECT id, domain_id, parent_id, cognitive_level_id, name, is_system_value, last_reviewed_at
         FROM domain_nodes
         ORDER BY domain_id, id",
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
         RETURNING id, domain_id, parent_id, cognitive_level_id, name, is_system_value, last_reviewed_at",
    )
    .bind(parent_id)
    .bind(name)
    .fetch_optional(pool)
    .await
}

pub async fn update_domain_node(
    pool: &PgPool,
    id: i32,
    name: &str,
) -> Result<Option<DomainNode>, sqlx::Error> {
    sqlx::query_as::<_, DomainNode>(
        "UPDATE domain_nodes
         SET name = $2
         WHERE id = $1
         RETURNING id, domain_id, parent_id, cognitive_level_id, name, is_system_value, last_reviewed_at",
    )
    .bind(id)
    .bind(name)
    .fetch_optional(pool)
    .await
}
