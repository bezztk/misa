use crate::models::item_type::ItemType;
use crate::models::skill::Skill;
use crate::models::skill_state::{SkillMetadata, SkillState, SkillStateRow};
use sqlx::PgPool;

pub async fn create_skill(
    pool: &PgPool,
    area_id: i32,
    state_id: Option<i32>,
    name: &str,
    description: Option<&str>,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let item_id: i32 =
        sqlx::query_scalar("INSERT INTO items (type) VALUES ($1) RETURNING id::INTEGER")
            .bind(ItemType::Skill as i32)
            .fetch_one(&mut *tx)
            .await?;

    sqlx::query(
        "INSERT INTO skills (item_id, area_id, state_id, name, description)
         VALUES ($1, $2, COALESCE($3, $4), $5, $6)",
    )
    .bind(item_id)
    .bind(area_id)
    .bind(state_id)
    .bind(SkillState::Planned as i32)
    .bind(name)
    .bind(description)
    .execute(&mut *tx)
    .await?;

    tx.commit().await?;

    Ok(item_id)
}

pub async fn get_skill(pool: &PgPool, item_id: i32) -> Result<Option<Skill>, sqlx::Error> {
    sqlx::query_as(
        "SELECT item_id, area_id, state_id, name, description
         FROM skills
         WHERE item_id = $1",
    )
    .bind(item_id)
    .fetch_optional(pool)
    .await
}

pub async fn get_skills(pool: &PgPool) -> Result<Vec<Skill>, sqlx::Error> {
    sqlx::query_as(
        "SELECT item_id, area_id, state_id, name, description
         FROM skills
         ORDER BY name, item_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn update_skill(
    pool: &PgPool,
    item_id: i32,
    state_id: Option<i32>,
    name: Option<&str>,
    description: Option<&str>,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE skills
         SET state_id = COALESCE($1, state_id),
             name = COALESCE($2, name),
             description = COALESCE($3, description)
         WHERE item_id = $4",
    )
    .bind(state_id)
    .bind(name)
    .bind(description)
    .bind(item_id)
    .execute(pool)
    .await?;

    Ok(result.rows_affected() > 0)
}

pub async fn delete_skill(pool: &PgPool, item_id: i32) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let result = sqlx::query("DELETE FROM skills WHERE item_id = $1")
        .bind(item_id)
        .execute(&mut *tx)
        .await?;

    if result.rows_affected() == 0 {
        tx.rollback().await?;
        return Ok(false);
    }

    sqlx::query("DELETE FROM items WHERE id = $1 AND type = $2")
        .bind(item_id)
        .bind(ItemType::Skill as i32)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;

    Ok(true)
}

pub async fn get_skill_metadata(pool: &PgPool) -> Result<SkillMetadata, sqlx::Error> {
    let states = sqlx::query_as::<_, SkillStateRow>("SELECT id, name FROM skill_states ORDER BY id")
        .fetch_all(pool)
        .await?;

    Ok(SkillMetadata { states })
}
