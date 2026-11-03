use crate::models::development_area::DevelopmentArea;
use sqlx::PgPool;

pub async fn create_development_area(
    pool: &PgPool,
    name: &str,
    description: Option<&str>,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO development_areas (name, description)
         VALUES ($1, $2)
         RETURNING id",
    )
    .bind(name)
    .bind(description)
    .fetch_one(pool)
    .await
}

pub async fn get_development_area(
    pool: &PgPool,
    id: i32,
) -> Result<Option<DevelopmentArea>, sqlx::Error> {
    sqlx::query_as("SELECT id, name, description FROM development_areas WHERE id = $1")
        .bind(id)
        .fetch_optional(pool)
        .await
}

pub async fn get_development_areas(pool: &PgPool) -> Result<Vec<DevelopmentArea>, sqlx::Error> {
    sqlx::query_as("SELECT id, name, description FROM development_areas ORDER BY name, id")
        .fetch_all(pool)
        .await
}

pub async fn update_development_area(
    pool: &PgPool,
    id: i32,
    name: Option<&str>,
    description: Option<&str>,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE development_areas
         SET name = COALESCE($1, name),
             description = COALESCE($2, description)
         WHERE id = $3",
    )
    .bind(name)
    .bind(description)
    .bind(id)
    .execute(pool)
    .await?;

    Ok(result.rows_affected() > 0)
}

pub async fn delete_development_area(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM development_areas WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
