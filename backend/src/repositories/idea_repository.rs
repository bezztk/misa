use crate::models::entity::EntityType;
use crate::models::idea::{
    ACTIVE_IDEA_STATE_ID, BOOKMARKED_IDEA_PHASE_ID, Idea, IdeaField, IdeaFieldUpdate,
};
use crate::repositories::{comment_repository, entity_repository};
use serde_json::json;
use sqlx::PgPool;

pub async fn get_ideas(pool: &PgPool) -> Result<Vec<Idea>, sqlx::Error> {
    let mut ideas = sqlx::query_as::<_, Idea>(
        "SELECT idea.id, idea.title, idea.content, idea.phase_id, idea.state_id,
                link.context_id, idea.completed_at, entity.created_at
         FROM ideas idea
         JOIN entities entity ON entity.id = idea.id
         LEFT JOIN context_entities link ON link.entity_id = idea.id
         ORDER BY entity.created_at DESC",
    )
    .fetch_all(pool)
    .await?;
    let entity_ids = ideas.iter().map(|idea| idea.id).collect::<Vec<_>>();
    let comments = comment_repository::get_comments_for_entities(pool, &entity_ids).await?;
    for idea in &mut ideas {
        idea.comments = comments
            .iter()
            .filter(|comment| comment.entity_id == idea.id)
            .cloned()
            .collect();
    }
    Ok(ideas)
}

pub async fn create_idea(pool: &PgPool, title: &str, content: &str) -> Result<i32, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let entity_id = entity_repository::create_entity(&mut transaction, EntityType::Idea).await?;
    sqlx::query(
        "INSERT INTO ideas (id, title, content, phase_id, state_id)
         VALUES ($1, $2, $3, $4, $5)",
    )
    .bind(entity_id)
    .bind(title)
    .bind(content)
    .bind(BOOKMARKED_IDEA_PHASE_ID)
    .bind(ACTIVE_IDEA_STATE_ID)
    .execute(&mut *transaction)
    .await?;
    transaction.commit().await?;
    Ok(entity_id)
}

pub async fn update_idea_context(
    pool: &PgPool,
    id: i32,
    context_id: Option<i32>,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let idea = sqlx::query_scalar::<_, i32>("SELECT id FROM ideas WHERE id = $1 FOR UPDATE")
        .bind(id)
        .fetch_optional(&mut *transaction)
        .await?;
    if idea.is_none() {
        return Ok(false);
    }

    sqlx::query("DELETE FROM context_entities WHERE entity_id = $1")
        .bind(id)
        .execute(&mut *transaction)
        .await?;
    if let Some(context_id) = context_id {
        sqlx::query("INSERT INTO context_entities (entity_id, context_id) VALUES ($1, $2)")
            .bind(id)
            .bind(context_id)
            .execute(&mut *transaction)
            .await?;
    }
    transaction.commit().await?;
    Ok(true)
}

pub async fn update_idea_field(
    pool: &PgPool,
    id: i32,
    update: IdeaFieldUpdate,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;

    let (field, old_value, new_value) = match update {
        IdeaFieldUpdate::Title(value) => {
            let Some(old_value) =
                sqlx::query_scalar::<_, String>("SELECT title FROM ideas WHERE id = $1 FOR UPDATE")
                    .bind(id)
                    .fetch_optional(&mut *transaction)
                    .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE ideas SET title = $2 WHERE id = $1")
                .bind(id)
                .bind(&value)
                .execute(&mut *transaction)
                .await?;
            (IdeaField::Title, json!(old_value), json!(value))
        }
        IdeaFieldUpdate::Content(value) => {
            let Some(old_value) = sqlx::query_scalar::<_, String>(
                "SELECT content FROM ideas WHERE id = $1 FOR UPDATE",
            )
            .bind(id)
            .fetch_optional(&mut *transaction)
            .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE ideas SET content = $2 WHERE id = $1")
                .bind(id)
                .bind(&value)
                .execute(&mut *transaction)
                .await?;
            (IdeaField::Content, json!(old_value), json!(value))
        }
        IdeaFieldUpdate::PhaseId(value) => {
            let Some(old_value) =
                sqlx::query_scalar::<_, i32>("SELECT phase_id FROM ideas WHERE id = $1 FOR UPDATE")
                    .bind(id)
                    .fetch_optional(&mut *transaction)
                    .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE ideas SET phase_id = $2 WHERE id = $1")
                .bind(id)
                .bind(value)
                .execute(&mut *transaction)
                .await?;
            (IdeaField::PhaseId, json!(old_value), json!(value))
        }
        IdeaFieldUpdate::StateId(value) => {
            let Some(old_value) =
                sqlx::query_scalar::<_, i32>("SELECT state_id FROM ideas WHERE id = $1 FOR UPDATE")
                    .bind(id)
                    .fetch_optional(&mut *transaction)
                    .await?
            else {
                return Ok(false);
            };
            if old_value == value {
                return Ok(true);
            }
            sqlx::query("UPDATE ideas SET state_id = $2 WHERE id = $1")
                .bind(id)
                .bind(value)
                .execute(&mut *transaction)
                .await?;
            (IdeaField::StateId, json!(old_value), json!(value))
        }
    };

    entity_repository::create_field_version(
        &mut transaction,
        id,
        EntityType::Idea,
        field as i32,
        old_value,
        new_value,
    )
    .await?;
    transaction.commit().await?;

    Ok(true)
}

pub async fn delete_idea(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM entities WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;

    Ok(result.rows_affected() > 0)
}
