use crate::api::handlers::task_handler;
use axum::{Router, routing::get};
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router {
    let api_routes = Router::new()
        .route(
            "/tasks/{id}",
            get(task_handler::get_task)
                .patch(task_handler::update_task)
                .delete(task_handler::delete_task),
        )
        .route(
            "/tasks",
            get(task_handler::get_tasks).post(task_handler::create_task),
        )
        .route("/tasks/metadata", get(task_handler::get_task_metadata));

    Router::new()
        .route("/health", get(health))
        .nest("/api", api_routes)
        .with_state(pool)
}

async fn health() -> &'static str {
    "OK"
}
