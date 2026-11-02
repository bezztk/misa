use axum::{Router, routing::get};
use axum::response::IntoResponse;
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router{
    Router::new()
        .with_state(pool)
        .route("/health", get(health))
}

async fn health() -> &'static str {
    "OK"
}