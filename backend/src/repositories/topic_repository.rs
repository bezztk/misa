use crate::models::document::Document;
use crate::models::entity::EntityType;
use crate::models::topic::{Topic, TopicGraph, TopicGraphEntry};
use crate::repositories::{development_area_repository, entity_repository, flashcard_repository};
use sqlx::PgPool;
use std::collections::HashMap;

pub async fn get_topic_graph(pool: &PgPool) -> Result<TopicGraph, sqlx::Error> {
    let development_areas = development_area_repository::get_development_areas(pool).await?;
    let topics = sqlx::query_as::<_, Topic>(
        "SELECT
            t.entity_id,
            e.created_at,
            e.updated_at,
            e.version,
            t.parent_topic_entity_id,
            t.development_area_id,
            t.name,
            t.understanding_level_id
         FROM topics t
         JOIN entities e ON e.id = t.entity_id
         WHERE NOT e.is_archived AND NOT e.is_deleted",
    )
    .fetch_all(pool)
    .await?;

    let mut documents = sqlx::query_as::<_, Document>(
        "SELECT
            d.entity_id,
            e.created_at,
            e.updated_at,
            e.version,
            d.topic_id,
            d.content,
            d.completeness_level_id,
            d.flashcard_status_id,
            d.verification_status_id,
            d.verification_level_id,
            d.last_verified_at
         FROM documents d
         JOIN entities e ON e.id = d.entity_id
         WHERE NOT e.is_archived AND NOT e.is_deleted",
    )
    .fetch_all(pool)
    .await?
    .into_iter()
    .map(|document| (document.topic_id, document))
    .collect::<HashMap<_, _>>();

    let mut flashcards = flashcard_repository::get_flashcards(pool)
        .await?
        .into_iter()
        .fold(HashMap::new(), |mut by_document, flashcard| {
            by_document
                .entry(flashcard.document_id)
                .or_insert_with(Vec::new)
                .push(flashcard);
            by_document
        });

    let topics = topics
        .into_iter()
        .map(|topic| {
            let document = documents.remove(&topic.entity_id);
            let document_flashcards = document
                .as_ref()
                .and_then(|document| flashcards.remove(&document.entity_id))
                .unwrap_or_default();
            TopicGraphEntry {
                topic,
                document,
                flashcards: document_flashcards,
            }
        })
        .collect();

    Ok(TopicGraph {
        development_areas,
        topics,
    })
}

pub async fn create_topic(
    pool: &PgPool,
    parent_topic_entity_id: Option<i32>,
    development_area_id: Option<i32>,
    name: &str,
) -> Result<i32, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let entity_id = entity_repository::create_entity(&mut tx, EntityType::Topic).await?;

    sqlx::query(
        "INSERT INTO topics
            (entity_id, parent_topic_entity_id, development_area_id, name,
             understanding_level_id)
         VALUES ($1, $2, $3, $4, $5)",
    )
    .bind(entity_id)
    .bind(parent_topic_entity_id)
    .bind(development_area_id)
    .bind(name)
    .bind(1)
    .execute(&mut *tx)
    .await?;

    let document_id = entity_repository::create_entity(&mut tx, EntityType::Document).await?;
    sqlx::query("INSERT INTO documents (entity_id, topic_id) VALUES ($1, $2)")
        .bind(document_id)
        .bind(entity_id)
        .execute(&mut *tx)
        .await?;

    tx.commit().await?;
    Ok(entity_id)
}

pub async fn update_topic(
    pool: &PgPool,
    entity_id: i32,
    parent_topic_entity_id: Option<i32>,
    development_area_id: Option<i32>,
    name: &str,
    understanding_level_id: i32,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;
    let result = sqlx::query(
        "UPDATE topics
         SET parent_topic_entity_id = $1,
             development_area_id = $2,
             name = $3,
             understanding_level_id = $4
         WHERE entity_id = $5",
    )
    .bind(parent_topic_entity_id)
    .bind(development_area_id)
    .bind(name)
    .bind(understanding_level_id)
    .bind(entity_id)
    .execute(&mut *tx)
    .await?;

    if result.rows_affected() == 0 {
        return Ok(false);
    }

    entity_repository::update_entity(&mut tx, entity_id).await?;
    tx.commit().await?;
    Ok(true)
}
