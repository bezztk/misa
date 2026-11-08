use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct TopicUnderstandingLevel {
    pub id: i32,
    pub name: String,
    pub display_name: Option<String>,
}
