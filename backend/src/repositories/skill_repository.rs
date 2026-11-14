use crate::models::skill::Skill;
use sqlx::PgPool;

pub async fn get_skills(pool: &PgPool, domain_node_id: i32) -> Result<Vec<Skill>, sqlx::Error> {
    sqlx::query_as::<_, Skill>(
        "SELECT id, domain_node_id, parent_skill_id, name, psychomotor_level_id
         FROM skills
         WHERE domain_node_id = $1
         ORDER BY id",
    )
    .bind(domain_node_id)
    .fetch_all(pool)
    .await
}

pub async fn create_skill(
    pool: &PgPool,
    domain_node_id: i32,
    parent_skill_id: Option<i32>,
    name: &str,
    psychomotor_level_id: Option<i32>,
) -> Result<Option<Skill>, sqlx::Error> {
    sqlx::query_as::<_, Skill>(
        "INSERT INTO skills (domain_node_id, parent_skill_id, name, psychomotor_level_id)
         SELECT $1, $2, $3, $4
         WHERE EXISTS (SELECT 1 FROM domain_nodes WHERE id = $1)
           AND ($2::INTEGER IS NULL OR EXISTS (
               SELECT 1 FROM skills WHERE id = $2 AND domain_node_id = $1
           ))
         RETURNING id, domain_node_id, parent_skill_id, name, psychomotor_level_id",
    )
    .bind(domain_node_id)
    .bind(parent_skill_id)
    .bind(name)
    .bind(psychomotor_level_id)
    .fetch_optional(pool)
    .await
}

pub async fn update_skill(
    pool: &PgPool,
    id: i32,
    name: &str,
    psychomotor_level_id: Option<i32>,
) -> Result<Option<Skill>, sqlx::Error> {
    sqlx::query_as::<_, Skill>(
        "UPDATE skills
         SET name = $2, psychomotor_level_id = $3
         WHERE id = $1
         RETURNING id, domain_node_id, parent_skill_id, name, psychomotor_level_id",
    )
    .bind(id)
    .bind(name)
    .bind(psychomotor_level_id)
    .fetch_optional(pool)
    .await
}
