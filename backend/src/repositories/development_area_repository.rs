use crate::models::development_area::DevelopmentArea;
use sqlx::PgPool;

pub async fn get_development_areas(pool: &PgPool) -> Result<Vec<DevelopmentArea>, sqlx::Error> {
    sqlx::query_as::<_, DevelopmentArea>(
        "SELECT id, name, background_color, banner_image_path, icon, sort_order
         FROM development_areas
         ORDER BY sort_order, id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_development_area(
    pool: &PgPool,
    name: &str,
    background_color: Option<&str>,
    banner_image_path: Option<&str>,
    icon: Option<&str>,
    sort_order: i32,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO development_areas
            (name, background_color, banner_image_path, icon, sort_order)
         VALUES ($1, $2, $3, $4, $5)
         RETURNING id",
    )
    .bind(name)
    .bind(background_color)
    .bind(banner_image_path)
    .bind(icon)
    .bind(sort_order)
    .fetch_one(pool)
    .await
}

pub async fn update_development_area(
    pool: &PgPool,
    id: i32,
    name: &str,
    background_color: Option<&str>,
    banner_image_path: Option<&str>,
    icon: Option<&str>,
    sort_order: i32,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE development_areas
         SET name = $1,
             background_color = $2,
             banner_image_path = $3,
             icon = $4,
             sort_order = $5
         WHERE id = $6",
    )
    .bind(name)
    .bind(background_color)
    .bind(banner_image_path)
    .bind(icon)
    .bind(sort_order)
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
