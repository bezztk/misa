use crate::api::handlers::flashcard_handler;
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
            axum::routing::patch(flashcard_handler::update_flashcard)
                .delete(flashcard_handler::delete_flashcard),
        )
        .route(
            "/flashcards/{id}/reviews",
            axum::routing::post(flashcard_handler::review_flashcard),
        )
        .route(
            "/flashcards/{id}/preview",
            get(flashcard_handler::preview_flashcard),
        )
        .route(
            "/flashcards",
            axum::routing::post(flashcard_handler::create_flashcard),
        );

    #[cfg(debug_assertions)]
    let api_routes = api_routes.route(
        "/dev/seed",
        axum::routing::post(crate::api::handlers::dev_handler::seed),
    );

    Router::new()
        .route("/health", get(health))
        .nest("/api", api_routes)
        .with_state(pool)
}

async fn health() -> &'static str {
    "OK"
}
