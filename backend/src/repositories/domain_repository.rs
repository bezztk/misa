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
