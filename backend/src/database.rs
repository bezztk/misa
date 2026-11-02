use sqlx::{Connection, PgConnection};
use crate::models::item_type::ItemTypeRow;
use crate::models::task_group::TaskGroup;
use crate::models::task_state::{TaskState, TaskStateRow};

pub async fn connect() -> Result<PgConnection, sqlx::Error> {
    let database_url = std::env::var("DATABASE_URL")
        .expect("DATABASE_URL must be set");

    PgConnection::connect(&database_url).await
}

// Get Lookup Tables
pub async fn get_item_types(connection: &mut PgConnection) -> Result<Vec<ItemTypeRow>, sqlx::Error> {
    sqlx::query_as::<_, ItemTypeRow>("SELECT id, name FROM types")
        .fetch_all(connection)
        .await
}

pub async fn get_task_groups(connection: &mut PgConnection) -> Result<Vec<TaskGroup>, sqlx::Error> {
    sqlx::query_as::<_, TaskGroup>("SELECT id, name FROM task_groups")
        .fetch_all(connection)
        .await
}

pub async fn get_task_states(connection: &mut PgConnection) -> Result<Vec<TaskStateRow>, sqlx::Error> {
    sqlx::query_as::<_, TaskStateRow>("SELECT id, name FROM task_states")
        .fetch_all(connection)
        .await
}