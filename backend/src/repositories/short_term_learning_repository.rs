use sqlx::{PgPool, types::Json};

use crate::models::short_term_learning::{
    COMPLETED, LEARNING, NEW, ShortTermLearningCard, ShortTermLearningCardContent,
    ShortTermLearningSet, UpdateShortTermLearningCard, UpdateShortTermLearningState,
};

pub async fn get_sets(pool: &PgPool) -> Result<Vec<ShortTermLearningSet>, sqlx::Error> {
    sqlx::query_as(
        "SELECT sets.id, sets.title, sets.bidirectional,
                COUNT(cards.id) AS card_count,
                COUNT(*) FILTER (WHERE cards.front_to_back_state_id = $1)
                    + COUNT(*) FILTER (WHERE cards.back_to_front_state_id = $1) AS new_count,
                COUNT(*) FILTER (WHERE cards.front_to_back_state_id = $2)
                    + COUNT(*) FILTER (WHERE cards.back_to_front_state_id = $2) AS learning_count,
                COUNT(*) FILTER (WHERE cards.front_to_back_state_id = $3)
                    + COUNT(*) FILTER (WHERE cards.back_to_front_state_id = $3) AS completed_count
         FROM short_term_learning_sets AS sets
         LEFT JOIN short_term_learning_cards AS cards ON cards.set_id = sets.id
         GROUP BY sets.id
         ORDER BY sets.id",
    )
    .bind(NEW)
    .bind(LEARNING)
    .bind(COMPLETED)
    .fetch_all(pool)
    .await
}

pub async fn create_set(
    pool: &PgPool,
    title: &str,
    bidirectional: bool,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar(
        "INSERT INTO short_term_learning_sets (title, bidirectional)
         VALUES ($1, $2)
         RETURNING id",
    )
    .bind(title)
    .bind(bidirectional)
    .fetch_one(pool)
    .await
}

pub async fn delete_set(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM short_term_learning_sets WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() != 0)
}

pub async fn reset_set(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    sqlx::query_scalar(
        "WITH reset AS (
            UPDATE short_term_learning_cards
            SET front_to_back_state_id = $2,
                back_to_front_state_id = CASE WHEN back_to_front_state_id IS NOT NULL THEN $2 END
            WHERE set_id = $1
         )
         SELECT EXISTS (SELECT 1 FROM short_term_learning_sets WHERE id = $1)",
    )
    .bind(id)
    .bind(NEW)
    .fetch_one(pool)
    .await
}

pub async fn get_cards(
    pool: &PgPool,
    set_id: i32,
) -> Result<Vec<ShortTermLearningCard>, sqlx::Error> {
    sqlx::query_as(
        "SELECT id, front, hint, back, front_to_back_state_id, back_to_front_state_id
         FROM short_term_learning_cards
         WHERE set_id = $1
         ORDER BY id",
    )
    .bind(set_id)
    .fetch_all(pool)
    .await
}

pub async fn import_cards(
    pool: &PgPool,
    set_id: i32,
    cards: &[ShortTermLearningCardContent],
) -> Result<bool, sqlx::Error> {
    sqlx::query_scalar(
        "WITH learning_set AS (
            SELECT id, bidirectional FROM short_term_learning_sets WHERE id = $1
         ), imported AS (
            INSERT INTO short_term_learning_cards (set_id, front, hint, back,
                                                    front_to_back_state_id, back_to_front_state_id)
            SELECT learning_set.id, card.data->>'front', card.data->>'hint', card.data->>'back',
                   $3, CASE WHEN learning_set.bidirectional THEN $3 END
            FROM learning_set
            CROSS JOIN jsonb_array_elements($2) WITH ORDINALITY AS card(data, position)
            ORDER BY card.position
         )
         SELECT EXISTS (SELECT 1 FROM learning_set)",
    )
    .bind(set_id)
    .bind(Json(cards))
    .bind(NEW)
    .fetch_one(pool)
    .await
}

pub async fn update_card(
    pool: &PgPool,
    id: i32,
    card: &UpdateShortTermLearningCard,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE short_term_learning_cards
         SET front = $2, hint = $3, back = $4, front_to_back_state_id = $5,
             back_to_front_state_id = $6
         WHERE id = $1 AND (back_to_front_state_id IS NULL) = ($6 IS NULL)",
    )
    .bind(id)
    .bind(&card.content.front)
    .bind(&card.content.hint)
    .bind(&card.content.back)
    .bind(card.front_to_back_state_id)
    .bind(card.back_to_front_state_id)
    .execute(pool)
    .await?;
    Ok(result.rows_affected() != 0)
}

pub async fn delete_card(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM short_term_learning_cards WHERE id = $1")
        .bind(id)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() != 0)
}

pub async fn update_state(
    pool: &PgPool,
    id: i32,
    update: UpdateShortTermLearningState,
) -> Result<bool, sqlx::Error> {
    let query = if update.reverse {
        "UPDATE short_term_learning_cards SET back_to_front_state_id = $2
         WHERE id = $1 AND back_to_front_state_id IS NOT NULL"
    } else {
        "UPDATE short_term_learning_cards SET front_to_back_state_id = $2 WHERE id = $1"
    };
    let result = sqlx::query(query)
        .bind(id)
        .bind(update.state_id)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() != 0)
}
