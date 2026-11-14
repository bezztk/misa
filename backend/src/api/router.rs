use crate::api::handlers::{
    document_handler, domain_handler, flashcard_handler, metadata_handler, task_handler,
};
use axum::{Router, routing::get};
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router {
    let api_routes = Router::new()
        .route("/domain-tree", get(domain_handler::get_domain_tree))
        .route(
            "/domain-nodes/{parent_id}/children",
            axum::routing::post(domain_handler::create_domain_node),
        )
        .route(
            "/domains/{domain_id}/nodes",
            axum::routing::post(domain_handler::create_root_domain_node),
        )
        .route(
            "/domain-nodes/{id}",
            axum::routing::patch(domain_handler::update_domain_node),
        )
        .route(
            "/domain-nodes/{node_id}/document",
            get(document_handler::get_document)
                .post(document_handler::create_document)
                .put(document_handler::update_document),
        )
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
            "/domain-nodes/{node_id}/flashcards",
            get(flashcard_handler::get_flashcards).post(flashcard_handler::create_flashcard),
        )
        .route("/metadata", get(metadata_handler::get_metadata))
        .route(
            "/tasks/{id}",
            axum::routing::patch(task_handler::update_task).delete(task_handler::delete_task),
        )
        .route(
            "/tasks",
            get(task_handler::get_tasks).post(task_handler::create_task),
        );

    Router::new()
        .route("/health", get(health))
        .nest("/api", api_routes)
        .with_state(pool)
}

async fn health() -> &'static str {
    "OK"
}
