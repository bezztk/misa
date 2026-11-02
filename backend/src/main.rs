use sqlx::Connection;

mod database;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    dotenvy::dotenv().ok();

    let connection = database::connect().await?;

    println!("Database connection established!");

    connection.close().await?;

    Ok(())
}