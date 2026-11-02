use sqlx::{PgPool};

mod database;
mod models;
pub mod repositories;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    dotenvy::dotenv().ok();

    let pool = database::connect().await?;

    println!("Database connection established!");

    test_crud(&pool).await?;

    pool.close().await;

    Ok(())
}

async fn test_crud(pool: &PgPool) -> Result<(), Box<dyn std::error::Error>> {
    // CREATE
    let id = repositories::task_repository::create_task(
        pool,
        "Rust lernen",
        Some(3),
        Some("Ownership und Borrowing verstehen"),
    ).await?;

    println!("Task erstellt: {id}");

    // READ
    let task = repositories::task_repository::get_task(
        pool,
        id,
    ).await?;

    println!("{task:#?}");

    // UPDATE
    repositories::task_repository::update_task(
        pool,
        id,
        "Rust Grundlagen abschließen",
        3,
    ).await?;

    // READ
    let task = repositories::task_repository::get_task(
        pool,
        id,
    ).await?;

    println!("{task:#?}");

    // DELETE
    repositories::task_repository::delete_task(
        pool,
        id,
    ).await?;

    // READ
    let task = repositories::task_repository::get_task(
        pool,
        id,
    ).await?;

    println!("{task:#?}");

    Ok(())
}