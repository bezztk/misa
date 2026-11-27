use crate::api::error::{ApiError, ApiResult};
use crate::api::response::IdResponse;
use crate::models::system_task::{
    CreateTaskRequest, TaskContextsRequest, TaskField, TaskFieldUpdate, TaskFieldUpdateRequest,
    TaskListItem,
};
use crate::repositories::task_repository;
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_tasks(State(pool): State<PgPool>) -> ApiResult<Json<Vec<TaskListItem>>> {
    Ok(Json(task_repository::get_tasks(&pool).await?))
}

pub async fn create_task(
    State(pool): State<PgPool>,
    Json(payload): Json<CreateTaskRequest>,
) -> ApiResult<(StatusCode, Json<IdResponse>)> {
    if !valid_name(&payload.name, 150)
        || (payload.domain_node_id.is_some() && !payload.context_ids.is_empty())
    {
        return Err(ApiError::BadRequest);
    }

    let id = task_repository::create_task(
        &pool,
        payload.name.trim(),
        &payload.context_ids,
        payload.priority_id,
        payload.domain_node_id,
    )
    .await?;

    Ok((StatusCode::CREATED, Json(IdResponse { id })))
}

pub async fn update_task_contexts(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<TaskContextsRequest>,
) -> ApiResult<StatusCode> {
    if !task_repository::update_task_contexts(&pool, id, &payload.context_ids).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn update_task_field(
    State(pool): State<PgPool>,
    Path((id, field_id)): Path<(i32, i32)>,
    Json(payload): Json<TaskFieldUpdateRequest>,
) -> ApiResult<StatusCode> {
    let field = TaskField::try_from(field_id).map_err(|_| ApiError::BadRequest)?;
    let update = match field {
        TaskField::Name => {
            let name = serde_json::from_value::<String>(payload.value)
                .map_err(|_| ApiError::BadRequest)?;
            if !valid_name(&name, 150) {
                return Err(ApiError::BadRequest);
            }
            TaskFieldUpdate::Name(name.trim().to_string())
        }
        TaskField::StateId => TaskFieldUpdate::StateId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
        TaskField::PriorityId => TaskFieldUpdate::PriorityId(
            serde_json::from_value(payload.value).map_err(|_| ApiError::BadRequest)?,
        ),
    };

    if !task_repository::update_task_field(&pool, id, update).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}

pub async fn delete_task(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<StatusCode> {
    if !task_repository::delete_task(&pool, id).await? {
        return Err(ApiError::NotFound);
    }

    Ok(StatusCode::NO_CONTENT)
}
