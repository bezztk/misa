use crate::models::pursuit::{
    Pursuit, PursuitDetails, PursuitRequirement, PursuitRequirementTypeId,
    UpdatePursuitRequirementsRequest,
};
use sqlx::PgPool;

pub async fn get_pursuits(
    pool: &PgPool,
    domain_node_id: i32,
) -> Result<Vec<PursuitDetails>, sqlx::Error> {
    let pursuits = sqlx::query_as::<_, Pursuit>(
        "SELECT id, domain_node_id, progression_stage_id, name, achieved_at
         FROM pursuits
         WHERE domain_node_id = $1
         ORDER BY id",
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
         JOIN pursuits p ON p.id = pr.pursuit_id
         WHERE p.domain_node_id = $1
         ORDER BY pr.id",
    )
    .bind(domain_node_id)
    .fetch_all(pool)
    .await?;

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
        })
        .collect())
}

pub async fn create_pursuit(
    pool: &PgPool,
    domain_node_id: i32,
    name: &str,
) -> Result<Option<PursuitDetails>, sqlx::Error> {
    let pursuit = sqlx::query_as::<_, Pursuit>(
        "INSERT INTO pursuits (domain_node_id, name)
         SELECT id, $2 FROM domain_nodes WHERE id = $1
         RETURNING id, domain_node_id, progression_stage_id, name, achieved_at",
    )
    .bind(domain_node_id)
    .bind(name)
    .fetch_optional(pool)
    .await?;

    Ok(pursuit.map(|pursuit| PursuitDetails {
        id: pursuit.id,
        domain_node_id: pursuit.domain_node_id,
        name: pursuit.name,
        requirements: Vec::new(),
    }))
}

pub async fn update_requirements(
    pool: &PgPool,
    pursuit_id: i32,
    requirements: &UpdatePursuitRequirementsRequest,
) -> Result<Option<i32>, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let domain_node_id = sqlx::query_scalar::<_, i32>(
        "SELECT domain_node_id FROM pursuits WHERE id = $1 FOR UPDATE",
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

    transaction.commit().await?;
    Ok(Some(domain_node_id))
}
