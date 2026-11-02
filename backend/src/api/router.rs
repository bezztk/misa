use axum::{Router, routing::{get, post}};
use sqlx::PgPool;
use crate::api::handlers::task_handler;

pub fn create_router(pool: PgPool) -> Router{
    Router::new()
        .route("/health", get(health))
        .route("/tasks/{id}", get(task_handler::get_task))
        .route("/tasks", post(task_handler::create_task))
        .with_state(pool)
}

async fn health() -> &'static str {
    "OK"
}