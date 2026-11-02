use sqlx::{Connection, PgConnection};

pub async fn connect() -> Result<PgConnection, sqlx::Error> {
    let database_url = std::env::var("DATABASE_URL")
        .expect("DATABASE_URL must be set");

    PgConnection::connect(&database_url).await
}