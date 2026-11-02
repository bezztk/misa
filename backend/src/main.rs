use sqlx::{Connection, PgConnection};
use crate::database::{get_item_types, get_task_groups, get_task_states};

mod database;
mod models;
pub mod repositories;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    dotenvy::dotenv().ok();

    let mut connection = database::connect().await?;

    println!("Database connection established!");

    test_crud(&mut connection).await?;

    let types = get_task_states(&mut connection).await?;
    types.iter().for_each(|t| {
        println!("{} - {}", t.id, t.name);
    });

    connection.close().await?;

    Ok(())
}

async fn test_crud(connection: &mut PgConnection) -> Result<(), Box<dyn std::error::Error>> {
    // CREATE
    let id = repositories::task_repository::create_task(
        connection,
        "Rust lernen",
        Some(3),
        Some("Ownership und Borrowing verstehen"),
    ).await?;

    println!("Task erstellt: {id}");

    // READ
    let task = repositories::task_repository::get_task(
        connection,
        id,
    ).await?;

    println!("{task:#?}");

    // UPDATE
    repositories::task_repository::update_task(
        connection,
        id,
        "Rust Grundlagen abschließen",
        3,
    ).await?;

    // READ
    let task = repositories::task_repository::get_task(
        connection,
        id,
    ).await?;

    println!("{task:#?}");

    // DELETE
    repositories::task_repository::delete_task(
        connection,
        id,
    ).await?;

    // READ
    let task = repositories::task_repository::get_task(
        connection,
        id,
    ).await?;

    println!("{task:#?}");

    Ok(())
}