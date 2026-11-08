use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentCompletenessLevel {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentFlashcardStatus {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentVerificationStatus {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DocumentVerificationLevel {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}
