use crate::models::entity::EntityType;
use crate::models::skill::{Skill, SkillField, SkillFieldUpdate};
use crate::repositories::{comment_repository, entity_repository};
use serde_json::json;
use sqlx::PgPool;

pub async fn get_skills(pool: &PgPool, domain_node_id: i32) -> Result<Vec<Skill>, sqlx::Error> {
    let mut skills = sqlx::query_as::<_, Skill>(
        "SELECT entity_id AS id, domain_node_id, parent_skill_id, name, psychomotor_level_id
         FROM skills
         WHERE domain_node_id = $1
         ORDER BY entity_id",
    )
    .bind(domain_node_id)
    .fetch_all(pool)
    .await?;
    let entity_ids = skills.iter().map(|skill| skill.id).collect::<Vec<_>>();
    let comments = comment_repository::get_comments_for_entities(pool, &entity_ids).await?;
    for skill in &mut skills {
        skill.comments = comments
            .iter()
            .filter(|comment| comment.entity_id == skill.id)
            .cloned()
            .collect();
    }
    Ok(skills)
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

pub async fn update_skill_field(
    pool: &PgPool,
    id: i32,
    update: SkillFieldUpdate,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;

    let (field, old_value, new_value) = match update {
        SkillFieldUpdate::Name(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, String>(
                "SELECT name FROM skills WHERE entity_id = $1 FOR UPDATE",
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
            sqlx::query("UPDATE skills SET name = $2 WHERE entity_id = $1")
                .bind(id)
                .bind(&value)
                .execute(&mut *transaction)
                .await?;
            (SkillField::Name, json!(old_value), json!(value))
        }
        SkillFieldUpdate::PsychomotorLevelId(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, Option<i32>>(
                "SELECT psychomotor_level_id FROM skills WHERE entity_id = $1 FOR UPDATE",
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
            sqlx::query("UPDATE skills SET psychomotor_level_id = $2 WHERE entity_id = $1")
                .bind(id)
                .bind(value)
                .execute(&mut *transaction)
                .await?;
            (
                SkillField::PsychomotorLevelId,
                json!(old_value),
                json!(value),
            )
        }
    };

    entity_repository::create_field_version(
        &mut transaction,
        id,
        EntityType::Skill,
        field as i32,
        old_value,
        new_value,
    )
    .await?;
    transaction.commit().await?;

    Ok(true)
}
