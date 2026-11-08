use crate::api::handlers::{
    calendar_entry_handler, document_handler, entity_handler, flashcard_handler, metadata_handler,
    task_handler, topic_handler,
};
use axum::{Router, routing::get};
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router {
    let api_routes = Router::new()
        .route(
            "/tasks/{entity_id}",
            get(task_handler::get_task).put(task_handler::update_task),
        )
        .route(
            "/tasks",
            get(task_handler::get_tasks).post(task_handler::create_task),
        )
        .route("/metadata", get(metadata_handler::get_metadata))
        .route(
            "/flashcards/{id}",
            axum::routing::patch(flashcard_handler::update_flashcard)
                .delete(flashcard_handler::delete_flashcard),
        )
        .route(
            "/flashcards",
            axum::routing::post(flashcard_handler::create_flashcard),
        )
        .route(
            "/calendar/{entity_id}",
            get(calendar_entry_handler::get_calendar_entry)
                .put(calendar_entry_handler::update_calendar_entry),
        )
        .route(
            "/calendar",
            get(calendar_entry_handler::get_calendar_entries)
                .post(calendar_entry_handler::create_calendar_entry),
        )
        .route(
            "/topics/{entity_id}",
            axum::routing::put(topic_handler::update_topic),
        )
        .route("/topics", axum::routing::post(topic_handler::create_topic))
        .route("/topic/graph", get(topic_handler::get_topic_graph))
        .route(
            "/topic/{entity_id}/graph",
            get(topic_handler::get_topic_graph_by_id),
        )
        .route(
            "/documents/{entity_id}",
            get(document_handler::get_document)
                .put(document_handler::update_document)
                .patch(document_handler::update_document_content),
        )
        .route(
            "/topics/{topic_id}/document",
            axum::routing::post(document_handler::create_document_for_topic),
        )
        .route(
            "/entities/{entity_id}/archive",
            axum::routing::put(entity_handler::archive_entity),
        )
        .route(
            "/entities/{entity_id}",
            axum::routing::delete(entity_handler::delete_entity),
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
