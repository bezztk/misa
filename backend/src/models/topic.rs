use crate::models::development_area::DevelopmentArea;
use crate::models::document::Document;
use crate::models::flashcard::Flashcard;
use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Topic {
    pub entity_id: i32,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub version: i32,
    pub parent_topic_entity_id: Option<i32>,
    pub development_area_id: Option<i32>,
    pub name: String,
    pub understanding_level_id: i32,
}

#[derive(Debug, Deserialize)]
pub struct CreateTopic {
    pub parent_topic_entity_id: Option<i32>,
    pub development_area_id: Option<i32>,
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct UpdateTopic {
    pub parent_topic_entity_id: Option<i32>,
    pub development_area_id: Option<i32>,
    pub name: String,
    pub understanding_level_id: i32,
}

#[derive(Debug, Serialize)]
pub struct TopicGraphEntry {
    pub topic: Topic,
    pub document: Option<Document>,
    pub flashcards: Vec<Flashcard>,
}

#[derive(Debug, Serialize)]
pub struct TopicGraph {
    pub development_areas: Vec<DevelopmentArea>,
    pub topics: Vec<TopicGraphEntry>,
}
