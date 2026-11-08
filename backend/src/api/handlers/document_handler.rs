use crate::models::document::{Document, UpdateDocument, UpdateDocumentContent};
use crate::repositories::document_repository;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_document(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
) -> Result<Json<Document>, StatusCode> {
    match document_repository::get_document(&pool, entity_id).await {
        Ok(Some(document)) => Ok(Json(document)),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to retrieve document: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn create_document_for_topic(
    State(pool): State<PgPool>,
    Path(topic_id): Path<i32>,
) -> Result<(StatusCode, Json<Document>), StatusCode> {
    match document_repository::get_document_by_topic(&pool, topic_id).await {
        Ok(Some(document)) => return Ok((StatusCode::OK, Json(document))),
        Ok(None) => {}
        Err(error) => {
            eprintln!("Failed to retrieve document: {error}");
            return Err(StatusCode::INTERNAL_SERVER_ERROR);
        }
    }

    let entity_id = match document_repository::create_document(&pool, topic_id).await {
        Ok(entity_id) => entity_id,
        Err(error) => {
            eprintln!("Failed to create document: {error}");
            return Err(StatusCode::INTERNAL_SERVER_ERROR);
        }
    };

    match document_repository::get_document(&pool, entity_id).await {
        Ok(Some(document)) => Ok((StatusCode::CREATED, Json(document))),
        Ok(None) => Err(StatusCode::INTERNAL_SERVER_ERROR),
        Err(error) => {
            eprintln!("Failed to retrieve created document: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_document_content(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
    Json(payload): Json<UpdateDocumentContent>,
) -> Result<StatusCode, StatusCode> {
    match document_repository::update_document_content(&pool, entity_id, &payload.content).await {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update document content: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn update_document(
    State(pool): State<PgPool>,
    Path(entity_id): Path<i32>,
    Json(payload): Json<UpdateDocument>,
) -> Result<StatusCode, StatusCode> {
    match document_repository::update_document(
        &pool,
        entity_id,
        &payload.content,
        payload.completeness_level_id,
        payload.flashcard_status_id,
        payload.verification_status_id,
        payload.verification_level_id,
        payload.last_verified_at,
    )
    .await
    {
        Ok(true) => Ok(StatusCode::NO_CONTENT),
        Ok(false) => Err(StatusCode::NOT_FOUND),
        Err(error) => {
            eprintln!("Failed to update document: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
