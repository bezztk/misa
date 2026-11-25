use sqlx::{AssertSqlSafe, PgPool, types::Json};

use crate::models::short_term_learning::{
    COMPLETED, LEARNING, NEW, RateShortTermLearningCard, ShortTermLearningCard,
    ShortTermLearningCardContent, ShortTermLearningCardRating, ShortTermLearningProgress,
    ShortTermLearningSet,
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
                back_to_front_state_id = CASE WHEN back_to_front_state_id IS NOT NULL THEN $2 END,
                front_to_back_last_rating_id = NULL,
                front_to_back_due_at = NULL,
                back_to_front_last_rating_id = NULL,
                back_to_front_due_at = NULL
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
        "SELECT id, front, hint, back, front_to_back_state_id, back_to_front_state_id,
                front_to_back_last_rating_id, front_to_back_due_at,
                back_to_front_last_rating_id, back_to_front_due_at
         FROM short_term_learning_cards
         WHERE set_id = $1
         ORDER BY id",
    )
    .bind(set_id)
    .fetch_all(pool)
    .await
}

pub async fn get_ratings(pool: &PgPool) -> Result<Vec<ShortTermLearningCardRating>, sqlx::Error> {
    sqlx::query_as(
        "SELECT id, display_name, interval_minutes
         FROM short_term_learning_card_ratings
         ORDER BY id",
    )
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
    card: &ShortTermLearningCardContent,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE short_term_learning_cards
         SET front = $2, hint = $3, back = $4
         WHERE id = $1",
    )
    .bind(id)
    .bind(&card.front)
    .bind(&card.hint)
    .bind(&card.back)
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

pub async fn rate_card(
    pool: &PgPool,
    id: i32,
    rating: RateShortTermLearningCard,
) -> Result<Option<ShortTermLearningProgress>, sqlx::Error> {
    let direction = if rating.reverse {
        "back_to_front"
    } else {
        "front_to_back"
    };
    let state = format!("{direction}_state_id");
    let last_rating = format!("{direction}_last_rating_id");
    let due_at = format!("{direction}_due_at");
    let completed = format!(
        "rating.interval_minutes IS NULL OR (rating.name = 'good' AND card.{last_rating} = rating.id)"
    );
    let query = format!(
        "UPDATE short_term_learning_cards AS card
         SET {state} = CASE WHEN {completed} THEN $3 ELSE $4 END,
             {last_rating} = rating.id,
             {due_at} = CASE WHEN {completed} THEN NULL
                 ELSE statement_timestamp() + make_interval(mins => rating.interval_minutes) END
         FROM short_term_learning_card_ratings AS rating
         WHERE card.id = $1 AND rating.id = $2 AND card.{state} <> $3
            AND (card.{due_at} IS NULL OR card.{due_at} <= statement_timestamp())
         RETURNING card.{state} AS state_id, card.{last_rating} AS last_rating_id,
                   card.{due_at} AS due_at"
    );
    // Only the fixed direction column names are interpolated.
    sqlx::query_as(AssertSqlSafe(query))
        .bind(id)
        .bind(rating.rating_id)
        .bind(COMPLETED)
        .bind(LEARNING)
        .fetch_optional(pool)
        .await
}
