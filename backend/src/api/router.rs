use crate::api::handlers::{calendar_entry_handler, task_handler};
use axum::{Router, routing::get};
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router {
    let api_routes = Router::new()
        .route(
            "/tasks/{entity_id}",
            get(task_handler::get_task)
                .put(task_handler::update_task)
                .delete(task_handler::delete_task),
        )
        .route(
            "/tasks",
            get(task_handler::get_tasks).post(task_handler::create_task),
        )
        .route("/tasks/metadata", get(task_handler::get_task_metadata))
        .route(
            "/calendar/{entity_id}",
            get(calendar_entry_handler::get_calendar_entry)
                .put(calendar_entry_handler::update_calendar_entry)
                .delete(calendar_entry_handler::delete_calendar_entry),
        )
        .route(
            "/calendar",
            get(calendar_entry_handler::get_calendar_entries)
                .post(calendar_entry_handler::create_calendar_entry),
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
