use crate::api::handlers::{development_area_handler, task_handler};
use axum::{Router, routing::get};
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router {
    let api_routes = Router::new()
        .route(
            "/development-areas/{id}",
            get(development_area_handler::get_development_area)
                .patch(development_area_handler::update_development_area)
                .delete(development_area_handler::delete_development_area),
        )
        .route(
            "/development-areas",
            get(development_area_handler::get_development_areas)
                .post(development_area_handler::create_development_area),
        )
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
