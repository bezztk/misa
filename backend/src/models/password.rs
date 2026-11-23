use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, sqlx::FromRow, Serialize)]
pub struct PasswordEntry {
    pub id: i32,
    pub category_id: Option<i32>,
    pub ciphertext: Vec<u8>,
    pub salt: Vec<u8>,
    pub nonce: Vec<u8>,
}

#[derive(Debug, Deserialize)]
pub struct SavePasswordEntryRequest {
    pub category_id: Option<i32>,
    pub ciphertext: Vec<u8>,
    pub salt: Vec<u8>,
    pub nonce: Vec<u8>,
}

#[derive(Debug, Clone, sqlx::FromRow, Serialize)]
pub struct PasswordCategory {
    pub id: i32,
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct SavePasswordCategoryRequest {
    pub name: String,
}
