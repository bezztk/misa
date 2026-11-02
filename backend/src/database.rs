use sqlx::{PgPool};
use sqlx::postgres::PgPoolOptions;

pub async fn connect() -> Result<PgPool, sqlx::Error> {
    let url = std::env::var("DATABASE_URL")
        .expect("DATABASE_URL missing");

    PgPoolOptions::new()
        .max_connections(5)
        .connect(&url)
        .await
}