use crate::api::error::{ApiError, ApiResult};
use crate::models::skill::{
    CreateSkillRequest, Skill, SkillField, SkillFieldUpdate, SkillFieldUpdateRequest,
};
use crate::repositories::skill_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_skills(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<Json<Vec<Skill>>> {
    Ok(Json(skill_repository::get_skills(&pool, node_id).await?))
}

pub async fn create_skill(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<CreateSkillRequest>,
) -> ApiResult<(StatusCode, Json<Skill>)> {
    if !valid_name(&payload.name, 150) {
        return Err(ApiError::BadRequest);
    }

    let skill = skill_repository::create_skill(
        &pool,
        node_id,
        payload.parent_skill_id,
        payload.name.trim(),
        payload.psychomotor_level_id,
    )
    .await?
    .ok_or(ApiError::NotFound)?;

    Ok((StatusCode::CREATED, Json(skill)))
}

pub async fn update_skill_field(
    State(pool): State<PgPool>,
    Path((id, field_id)): Path<(i32, i32)>,
    Json(payload): Json<SkillFieldUpdateRequest>,
) -> ApiResult<StatusCode> {
    let field = SkillField::try_from(field_id).map_err(|_| ApiError::BadRequest)?;
    let update = match field {
        SkillField::Name => {
            let name = serde_json::from_value::<String>(payload.value)
                .map_err(|_| ApiError::BadRequest)?;
            if !valid_name(&name, 150) {
                return Err(ApiError::BadRequest);
            }
            SkillFieldUpdate::Name(name.trim().to_string())
        }
        SkillField::PsychomotorLevelId => SkillFieldUpdate::PsychomotorLevelId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
    };

    if !skill_repository::update_skill_field(&pool, id, update).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}
