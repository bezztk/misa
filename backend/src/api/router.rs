use crate::api::handlers::{flashcard_handler, metadata_handler};
use axum::{Router, routing::get};
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router {
    let api_routes = Router::new()
        .route(
            "/flashcards/due",
            get(flashcard_handler::get_due_flashcards),
        )
        .route(
            "/flashcards/{id}",
            axum::routing::delete(flashcard_handler::delete_flashcard),
        )
        .route(
            "/flashcards/{id}/reviews",
            axum::routing::post(flashcard_handler::review_flashcard),
        )
        .route(
            "/domain-nodes/{node_id}/flashcards",
            axum::routing::post(flashcard_handler::create_flashcard),
        )
        .route("/metadata", get(metadata_handler::get_metadata));

    Router::new()
        .route("/health", get(health))
        .nest("/api", api_routes)
        .with_state(pool)
}

async fn health() -> &'static str {
    "OK"
}
