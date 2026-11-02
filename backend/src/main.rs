use sqlx::Connection;
use crate::database::{get_item_types, get_task_groups, get_task_states};

mod database;
mod models;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    dotenvy::dotenv().ok();

    let mut connection = database::connect().await?;

    println!("Database connection established!");

    let types = get_task_states(&mut connection).await?;
    types.iter().for_each(|t| {
        println!("{} - {}", t.id, t.name);
    });

    connection.close().await?;

    Ok(())
}