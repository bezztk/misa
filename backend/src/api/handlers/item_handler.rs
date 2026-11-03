use crate::models::item::{AllowedTypeRelation, ItemHierarchy, UpdateItemArchive, UpdateItemParent};
use crate::models::item_type::ItemType;
use crate::repositories::item_repository::{self, UpdateParentResult};
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_allowed_type_relations() -> Json<Vec<AllowedTypeRelation>> {
    let mut relations = Vec::new();

    for child_type in ItemType::ALL {
        for parent_type in child_type.allowed_parent_types() {
            relations.push(AllowedTypeRelation {
                parent_type: parent_type.name(),
                child_type: child_type.name(),
            });
        }
    }

    Json(relations)
}

pub async fn get_item_hierarchy(
    State(pool): State<PgPool>,
) -> Result<Json<Vec<ItemHierarchy>>, StatusCode> {
    match item_repository::get_item_hierarchy(&pool).await {
        Ok(hierarchy) => Ok(Json(hierarchy)),
        Err(error) => {
            eprintln!("Failed to retrieve item hierarchy: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_item_parent(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateItemParent>,
) -> Result<StatusCode, StatusCode> {
    match item_repository::update_item_parent(&pool, id, payload.parent_id).await {
        Ok(UpdateParentResult::Updated) => Ok(StatusCode::NO_CONTENT),
        Ok(UpdateParentResult::NotFound) => Err(StatusCode::NOT_FOUND),
        Ok(UpdateParentResult::Invalid) => Err(StatusCode::BAD_REQUEST),
        Err(error) => {
            eprintln!("Failed to update item parent: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_item_archive(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateItemArchive>,
) -> Result<StatusCode, StatusCode> {
    match item_repository::update_item_archive(&pool, id, payload.is_archived).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update item archive: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
