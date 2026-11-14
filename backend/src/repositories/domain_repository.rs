use crate::models::domain::DomainNode;
use sqlx::PgPool;

pub async fn get_domain_nodes(
    pool: &PgPool,
    domain_id: i32,
) -> Result<Vec<DomainNode>, sqlx::Error> {
    sqlx::query_as::<_, DomainNode>(
        "SELECT id, domain_id, parent_id, cognitive_level_id, name, is_system_value, last_reviewed_at
         FROM domain_nodes
         WHERE domain_id = $1
         ORDER BY id",
    )
    .bind(domain_id)
    .fetch_all(pool)
    .await
}
