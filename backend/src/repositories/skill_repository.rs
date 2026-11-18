use crate::models::entity::EntityType;
use crate::models::skill::Skill;
use crate::repositories::entity_repository;
use sqlx::PgPool;

pub async fn get_skills(pool: &PgPool, domain_node_id: i32) -> Result<Vec<Skill>, sqlx::Error> {
    sqlx::query_as::<_, Skill>(
        "SELECT entity_id AS id, domain_node_id, parent_skill_id, name, psychomotor_level_id
         FROM skills
         WHERE domain_node_id = $1
         ORDER BY entity_id",
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
    let mut transaction = pool.begin().await?;
    let is_valid = sqlx::query_scalar::<_, bool>(
        "SELECT EXISTS (SELECT 1 FROM domain_nodes WHERE entity_id = $1)
           AND ($2::INTEGER IS NULL OR EXISTS (
               SELECT 1 FROM skills WHERE entity_id = $2 AND domain_node_id = $1
           ))",
    )
    .bind(domain_node_id)
    .bind(parent_skill_id)
    .fetch_one(&mut *transaction)
    .await?;
    if !is_valid {
        return Ok(None);
    }
    let entity_id = entity_repository::create_entity(&mut transaction, EntityType::Skill).await?;
    let skill = sqlx::query_as::<_, Skill>(
        "INSERT INTO skills
            (entity_id, domain_node_id, parent_skill_id, name, psychomotor_level_id)
         VALUES ($1, $2, $3, $4, $5)
         RETURNING entity_id AS id, domain_node_id, parent_skill_id, name, psychomotor_level_id",
    )
    .bind(entity_id)
    .bind(domain_node_id)
    .bind(parent_skill_id)
    .bind(name)
    .bind(psychomotor_level_id)
    .fetch_one(&mut *transaction)
    .await?;
    transaction.commit().await?;
    Ok(Some(skill))
}

pub async fn update_skill(
    pool: &PgPool,
    id: i32,
    name: &str,
    psychomotor_level_id: Option<i32>,
) -> Result<Option<Skill>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let skill = sqlx::query_as::<_, Skill>(
        "UPDATE skills
         SET name = $2, psychomotor_level_id = $3
         WHERE entity_id = $1
         RETURNING entity_id AS id, domain_node_id, parent_skill_id, name, psychomotor_level_id",
    )
    .bind(id)
    .bind(name)
    .bind(psychomotor_level_id)
    .fetch_optional(&mut *transaction)
    .await?;
    if skill.is_some() {
        sqlx::query("UPDATE entities SET version = version + 1, updated_at = NOW() WHERE id = $1")
            .bind(id)
            .execute(&mut *transaction)
            .await?;
    }
    transaction.commit().await?;
    Ok(skill)
}
