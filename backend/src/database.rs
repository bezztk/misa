use sqlx::{Connection, PgConnection};
use crate::models::item_type::ItemTypeRow;

pub async fn connect() -> Result<PgConnection, sqlx::Error> {
    let database_url = std::env::var("DATABASE_URL")
        .expect("DATABASE_URL must be set");

    PgConnection::connect(&database_url).await
}

pub async fn get_item_types(connection: &mut PgConnection) -> Result<Vec<ItemTypeRow>, sqlx::Error> {
    sqlx::query_as::<_, ItemTypeRow>("SELECT id, name FROM types")
        .fetch_all(connection)
        .await
}