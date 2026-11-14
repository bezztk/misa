use crate::models::metadata::Metadata;
use crate::repositories::metadata_repository;
use axum::Json;
use axum::extract::State;
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_metadata(State(pool): State<PgPool>) -> Result<Json<Metadata>, StatusCode> {
    match metadata_repository::get_metadata(&pool).await {
        Ok(metadata) => Ok(Json(metadata)),
        Err(error) => {
            eprintln!("Failed to retrieve metadata: {error}");
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
