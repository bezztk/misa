use crate::models::entity::EntityType;
use crate::models::progression::{
    Progression, ProgressionDetails, ProgressionPursuit, ProgressionStage, ProgressionStageDetails,
};
use crate::repositories::entity_repository;
use sqlx::PgPool;

pub async fn get_progressions(pool: &PgPool) -> Result<Vec<ProgressionDetails>, sqlx::Error> {
    let progressions = sqlx::query_as::<_, Progression>(
        "SELECT entity_id AS id, name, personal_relevance_id, lasting_impact_id, motivation, desired_outcome
         FROM progressions
         ORDER BY entity_id",
    )
    .fetch_all(pool)
    .await?;
    let stages = sqlx::query_as::<_, ProgressionStage>(
        "SELECT id, progression_id, stage, name, description, is_completed, completed_at
         FROM progression_stages
         ORDER BY progression_id, stage",
    )
    .fetch_all(pool)
    .await?;
    let pursuits = get_pursuits(pool).await?;

    Ok(progressions
        .into_iter()
        .map(|progression| ProgressionDetails {
            id: progression.id,
            name: progression.name,
            personal_relevance_id: progression.personal_relevance_id,
            lasting_impact_id: progression.lasting_impact_id,
            stages: stages
                .iter()
                .filter(|stage| stage.progression_id == progression.id)
                .map(|stage| ProgressionStageDetails {
                    id: stage.id,
                    stage: stage.stage,
                    name: stage.name.clone(),
                    pursuits: pursuits
                        .iter()
                        .filter(|pursuit| pursuit.progression_stage_id == Some(stage.id))
                        .cloned()
                        .collect(),
                })
                .collect(),
        })
        .collect())
}

pub async fn get_pursuits(pool: &PgPool) -> Result<Vec<ProgressionPursuit>, sqlx::Error> {
    sqlx::query_as::<_, ProgressionPursuit>(
        "SELECT entity_id AS id, name, progression_stage_id FROM pursuits ORDER BY name, entity_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn get_available_pursuits(pool: &PgPool) -> Result<Vec<ProgressionPursuit>, sqlx::Error> {
    sqlx::query_as::<_, ProgressionPursuit>(
        "SELECT entity_id AS id, name, progression_stage_id
         FROM pursuits
         WHERE progression_stage_id IS NULL
         ORDER BY name, entity_id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_progression(pool: &PgPool, name: &str) -> Result<(), sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let entity_id =
        entity_repository::create_entity(&mut transaction, EntityType::Progression).await?;
    sqlx::query("INSERT INTO progressions (entity_id, name) VALUES ($1, $2)")
        .bind(entity_id)
        .bind(name)
        .execute(&mut *transaction)
        .await?;
    transaction.commit().await?;
    Ok(())
}

pub async fn update_progression(
    pool: &PgPool,
    id: i32,
    name: &str,
    personal_relevance_id: Option<i32>,
    lasting_impact_id: Option<i32>,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE progressions
         SET name = $2, personal_relevance_id = $3, lasting_impact_id = $4
         WHERE entity_id = $1",
    )
    .bind(id)
    .bind(name)
    .bind(personal_relevance_id)
    .bind(lasting_impact_id)
    .execute(&mut *transaction)
    .await?;
    if result.rows_affected() > 0 {
        sqlx::query("UPDATE entities SET version = version + 1, updated_at = NOW() WHERE id = $1")
            .bind(id)
            .execute(&mut *transaction)
            .await?;
    }
    transaction.commit().await?;
    Ok(result.rows_affected() > 0)
}

pub async fn create_stage(
    pool: &PgPool,
    progression_id: i32,
    name: &str,
) -> Result<bool, sqlx::Error> {
    Ok(sqlx::query(
        "INSERT INTO progression_stages (progression_id, stage, name)
         SELECT $1, COALESCE(MAX(stage), 0) + 1, $2
         FROM progression_stages
         WHERE progression_id = $1
         HAVING EXISTS (SELECT 1 FROM progressions WHERE entity_id = $1)",
    )
    .bind(progression_id)
    .bind(name)
    .execute(pool)
    .await?
    .rows_affected()
        > 0)
}

pub async fn assign_pursuit(
    pool: &PgPool,
    stage_id: i32,
    pursuit_id: i32,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE pursuits
         SET progression_stage_id = $1
         WHERE entity_id = $2
           AND progression_stage_id IS NULL
           AND EXISTS (SELECT 1 FROM progression_stages WHERE id = $1)",
    )
    .bind(stage_id)
    .bind(pursuit_id)
    .execute(&mut *transaction)
    .await?;
    if result.rows_affected() > 0 {
        sqlx::query("UPDATE entities SET version = version + 1, updated_at = NOW() WHERE id = $1")
            .bind(pursuit_id)
            .execute(&mut *transaction)
            .await?;
    }
    transaction.commit().await?;
    Ok(result.rows_affected() > 0)
}
