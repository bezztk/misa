use crate::api::router::create_router;

pub mod api;
mod database;
mod models;
pub mod repositories;
mod services;
mod validation;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    dotenvy::dotenv().ok();

    let pool = database::connect().await?;

    println!("Database connection established!");

    let rss_client = services::rss_service::create_http_client()?;
    services::rss_service::start_fetch_loop(pool.clone(), rss_client);

    let app = create_router(pool.clone());

    let listener = tokio::net::TcpListener::bind("127.0.0.1:3000").await?;

    println!("Server started on http://{}", listener.local_addr()?);

    axum::serve(listener, app).await?;

    pool.close().await;

    Ok(())
}
