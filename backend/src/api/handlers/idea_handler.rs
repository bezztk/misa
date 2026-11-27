use crate::api::error::{ApiError, ApiResult};
use crate::api::response::IdResponse;
use crate::models::idea::{
    CreateIdeaRequest, Idea, IdeaContextRequest, IdeaField, IdeaFieldUpdate, IdeaFieldUpdateRequest,
};
use crate::repositories::idea_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_ideas(State(pool): State<PgPool>) -> ApiResult<Json<Vec<Idea>>> {
    Ok(Json(idea_repository::get_ideas(&pool).await?))
}

pub async fn create_idea(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateIdeaRequest>,
) -> ApiResult<(StatusCode, Json<IdResponse>)> {
    if !valid_name(&payload.title, 150) {
        return Err(ApiError::BadRequest);
    }

    let id = idea_repository::create_idea(&pool, payload.title.trim(), &payload.content).await?;
    Ok((StatusCode::CREATED, Json(IdResponse { id })))
}

pub async fn update_idea_field(
    State(pool): State<PgPool>,
    Path((id, field_id)): Path<(i32, i32)>,
    Json(payload): Json<IdeaFieldUpdateRequest>,
) -> ApiResult<StatusCode> {
    let field = IdeaField::try_from(field_id).map_err(|_| ApiError::BadRequest)?;
    let update = match field {
        IdeaField::Title => {
            let title = serde_json::from_value::<String>(payload.value)
                .map_err(|_| ApiError::BadRequest)?;
            if !valid_name(&title, 150) {
                return Err(ApiError::BadRequest);
            }
            IdeaFieldUpdate::Title(title.trim().to_string())
        }
        IdeaField::Content => IdeaFieldUpdate::Content(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
        IdeaField::PhaseId => IdeaFieldUpdate::PhaseId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
        IdeaField::StateId => IdeaFieldUpdate::StateId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
    };

    if !idea_repository::update_idea_field(&pool, id, update).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}

pub async fn update_idea_context(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<IdeaContextRequest>,
) -> ApiResult<StatusCode> {
    if !idea_repository::update_idea_context(&pool, id, payload.context_id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn delete_idea(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<StatusCode> {
    if !idea_repository::delete_idea(&pool, id).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}
