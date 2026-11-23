use sqlx::PgPool;

use crate::models::password::{PasswordCategory, PasswordEntry};

pub async fn get_entries(pool: &PgPool) -> Result<Vec<PasswordEntry>, sqlx::Error> {
    sqlx::query_as::<_, PasswordEntry>(
        "SELECT id, category_id, ciphertext, salt, nonce
         FROM password_entries
         ORDER BY id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_entry(
    pool: &PgPool,
    category_id: Option<i32>,
    ciphertext: &[u8],
    salt: &[u8],
    nonce: &[u8],
) -> Result<PasswordEntry, sqlx::Error> {
    sqlx::query_as::<_, PasswordEntry>(
        "INSERT INTO password_entries (category_id, ciphertext, salt, nonce)
         VALUES ($1, $2, $3, $4)
         RETURNING id, category_id, ciphertext, salt, nonce",
    )
    .bind(category_id)
    .bind(ciphertext)
    .bind(salt)
    .bind(nonce)
    .fetch_one(pool)
    .await
}

pub async fn update_entry(
    pool: &PgPool,
    id: i32,
    category_id: Option<i32>,
    ciphertext: &[u8],
    salt: &[u8],
    nonce: &[u8],
) -> Result<Option<PasswordEntry>, sqlx::Error> {
    sqlx::query_as::<_, PasswordEntry>(
        "UPDATE password_entries
         SET category_id = $2, ciphertext = $3, salt = $4, nonce = $5
         WHERE id = $1
         RETURNING id, category_id, ciphertext, salt, nonce",
    )
    .bind(id)
    .bind(category_id)
    .bind(ciphertext)
    .bind(salt)
    .bind(nonce)
    .fetch_optional(pool)
    .await
}

pub async fn delete_entry(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM password_entries WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() > 0)
}

pub async fn get_categories(pool: &PgPool) -> Result<Vec<PasswordCategory>, sqlx::Error> {
    sqlx::query_as::<_, PasswordCategory>(
        "SELECT id, name
         FROM password_categories
         ORDER BY name, id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_category(pool: &PgPool, name: &str) -> Result<PasswordCategory, sqlx::Error> {
    sqlx::query_as::<_, PasswordCategory>(
        "INSERT INTO password_categories (name)
         VALUES ($1)
         RETURNING id, name",
    )
    .bind(name)
    .fetch_one(pool)
    .await
}

pub async fn update_category(
    pool: &PgPool,
    id: i32,
    name: &str,
) -> Result<Option<PasswordCategory>, sqlx::Error> {
    sqlx::query_as::<_, PasswordCategory>(
        "UPDATE password_categories
         SET name = $2
         WHERE id = $1
         RETURNING id, name",
    )
    .bind(id)
    .bind(name)
    .fetch_optional(pool)
    .await
}

pub async fn delete_category(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM password_categories WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() > 0)
}
