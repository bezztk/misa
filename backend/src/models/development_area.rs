use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct DevelopmentArea {
    pub id: i32,
    pub name: String,
    pub background_color: Option<String>,
    pub banner_image_path: Option<String>,
    pub icon: Option<String>,
    pub sort_order: i32,
}

#[derive(Debug, Deserialize)]
pub struct CreateDevelopmentArea {
    pub name: String,
    pub background_color: Option<String>,
    pub banner_image_path: Option<String>,
    pub icon: Option<String>,
    pub sort_order: i32,
}

#[derive(Debug, Deserialize)]
pub struct UpdateDevelopmentArea {
    pub name: String,
    pub background_color: Option<String>,
    pub banner_image_path: Option<String>,
    pub icon: Option<String>,
    pub sort_order: i32,
}
