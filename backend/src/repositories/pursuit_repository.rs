use crate::models::entity::EntityType;
use crate::models::pursuit::{
    Pursuit, PursuitDetails, PursuitRequirement, PursuitRequirementTypeId,
    UpdatePursuitRequirementsRequest,
};
use crate::repositories::{comment_repository, entity_repository};
use sqlx::PgPool;

pub async fn get_pursuits(
    pool: &PgPool,
    domain_node_id: i32,
) -> Result<Vec<PursuitDetails>, sqlx::Error> {
    let pursuits = sqlx::query_as::<_, Pursuit>(
        "SELECT entity_id AS id, domain_node_id, progression_stage_id, name, achieved_at
         FROM pursuits
         WHERE domain_node_id = $1
         ORDER BY entity_id",
    )
    .bind(domain_node_id)
    .fetch_all(pool)
    .await?;

    let requirements = sqlx::query_as::<_, PursuitRequirement>(
        "SELECT pr.id, pr.pursuit_id, pr.requirement_type_id, pr.skill_id,
                pr.cognitive_level_id, pr.psychomotor_level_id,
                pr.completeness_level_id, pr.flashcard_status_id,
                pr.verification_status_id, pr.verification_level_id
         FROM pursuit_requirements pr
         JOIN pursuits p ON p.entity_id = pr.pursuit_id
         WHERE p.domain_node_id = $1
         ORDER BY pr.id",
    )
    .bind(domain_node_id)
    .fetch_all(pool)
    .await?;

    let entity_ids = pursuits
        .iter()
        .map(|pursuit| pursuit.id)
        .collect::<Vec<_>>();
    let comments = comment_repository::get_comments_for_entities(pool, &entity_ids).await?;

    Ok(pursuits
        .into_iter()
        .map(|pursuit| PursuitDetails {
            id: pursuit.id,
            domain_node_id: pursuit.domain_node_id,
            name: pursuit.name,
            requirements: requirements
                .iter()
                .filter(|requirement| requirement.pursuit_id == pursuit.id)
                .cloned()
                .collect(),
            comments: comments
                .iter()
                .filter(|comment| comment.entity_id == pursuit.id)
                .cloned()
                .collect(),
        })
        .collect())
}

pub async fn create_pursuit(
    pool: &PgPool,
    domain_node_id: i32,
    name: &str,
) -> Result<Option<PursuitDetails>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let node_exists = sqlx::query_scalar::<_, bool>(
        "SELECT EXISTS (SELECT 1 FROM domain_nodes WHERE entity_id = $1)",
    )
    .bind(domain_node_id)
    .fetch_one(&mut *transaction)
    .await?;
    if !node_exists {
        return Ok(None);
    }
    let entity_id = entity_repository::create_entity(&mut transaction, EntityType::Pursuit).await?;
    let pursuit = sqlx::query_as::<_, Pursuit>(
        "INSERT INTO pursuits (entity_id, domain_node_id, name)
         VALUES ($1, $2, $3)
         RETURNING entity_id AS id, domain_node_id, progression_stage_id, name, achieved_at",
    )
    .bind(entity_id)
    .bind(domain_node_id)
    .bind(name)
    .fetch_one(&mut *transaction)
    .await?;
    transaction.commit().await?;

    Ok(Some(PursuitDetails {
        id: pursuit.id,
        domain_node_id: pursuit.domain_node_id,
        name: pursuit.name,
        requirements: Vec::new(),
        comments: Vec::new(),
    }))
}

pub async fn update_requirements(
    pool: &PgPool,
    pursuit_id: i32,
    requirements: &UpdatePursuitRequirementsRequest,
) -> Result<Option<i32>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let domain_node_id = sqlx::query_scalar::<_, i32>(
        "SELECT domain_node_id FROM pursuits WHERE entity_id = $1 FOR UPDATE",
    )
    .bind(pursuit_id)
    .fetch_optional(&mut *transaction)
    .await?;

    let Some(domain_node_id) = domain_node_id else {
        return Ok(None);
    };

    sqlx::query("DELETE FROM pursuit_requirements WHERE pursuit_id = $1")
        .bind(pursuit_id)
        .execute(&mut *transaction)
        .await?;

    for (requirement_type_id, cognitive, completeness, flashcard, verification, level) in [
        (
            PursuitRequirementTypeId::CognitiveLevel as i32,
            requirements.cognitive_level_id,
            None,
            None,
            None,
            None,
        ),
        (
            PursuitRequirementTypeId::CompletenessLevel as i32,
            None,
            requirements.completeness_level_id,
            None,
            None,
            None,
        ),
        (
            PursuitRequirementTypeId::FlashcardStatus as i32,
            None,
            None,
            requirements.flashcard_status_id,
            None,
            None,
        ),
        (
            PursuitRequirementTypeId::VerificationStatus as i32,
            None,
            None,
            None,
            requirements.verification_status_id,
            None,
        ),
        (
            PursuitRequirementTypeId::VerificationLevel as i32,
            None,
            None,
            None,
            None,
            requirements.verification_level_id,
        ),
    ] {
        if cognitive.is_some()
            || completeness.is_some()
            || flashcard.is_some()
            || verification.is_some()
            || level.is_some()
        {
            sqlx::query(
                "INSERT INTO pursuit_requirements
                    (pursuit_id, requirement_type_id, cognitive_level_id,
                     completeness_level_id, flashcard_status_id,
                     verification_status_id, verification_level_id)
                 VALUES ($1, $2, $3, $4, $5, $6, $7)",
            )
            .bind(pursuit_id)
            .bind(requirement_type_id)
            .bind(cognitive)
            .bind(completeness)
            .bind(flashcard)
            .bind(verification)
            .bind(level)
            .execute(&mut *transaction)
            .await?;
        }
    }

    for skill in &requirements.skills {
        sqlx::query(
            "INSERT INTO pursuit_requirements
                (pursuit_id, requirement_type_id, skill_id, psychomotor_level_id)
             VALUES ($1, $2, $3, $4)",
        )
        .bind(pursuit_id)
        .bind(PursuitRequirementTypeId::PsychomotorLevel as i32)
        .bind(skill.skill_id)
        .bind(skill.psychomotor_level_id)
        .execute(&mut *transaction)
        .await?;
    }

    sqlx::query("UPDATE entities SET version = version + 1, updated_at = NOW() WHERE id = $1")
        .bind(pursuit_id)
        .execute(&mut *transaction)
        .await?;

    transaction.commit().await?;
    Ok(Some(domain_node_id))
}
