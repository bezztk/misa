use crate::models::entity::EntityType;
use crate::models::media::{
    COMPLETED_WATCH_STATUS, Media, MediaEpisode, MediaIdentity, MediaMetadata, MediaSeason,
    SeasonMetadata, UpdateMediaRequest,
};
use crate::repositories::entity_repository;
use sqlx::{PgPool, Postgres, QueryBuilder, Transaction};

const SELECT_MEDIA: &str = "SELECT media.*, media_types.name AS media_type,
        ARRAY(
            SELECT genre.name
            FROM media_genre_assignments assignment
            JOIN media_genres genre ON genre.id = assignment.genre_id
            WHERE assignment.media_id = media.entity_id
            ORDER BY genre.name
        ) AS genres,
        (SELECT COUNT(*) FROM media_seasons season WHERE season.media_id = media.entity_id)
            AS season_count,
        (SELECT COUNT(*) FROM media_seasons season
         JOIN media_episodes episode ON episode.season_id = season.id
         WHERE season.media_id = media.entity_id) AS episode_count
     FROM media
     JOIN media_types ON media_types.id = media.media_type_id
     WHERE ($1::integer IS NULL OR media.entity_id = $1)
       AND ($2::text IS NULL
            OR STRPOS(LOWER(media.title), LOWER($2)) > 0
            OR STRPOS(LOWER(media.original_title), LOWER($2)) > 0)
     ORDER BY LOWER(media.title), media.entity_id";

pub async fn get_all(pool: &PgPool, query: Option<&str>) -> Result<Vec<Media>, sqlx::Error> {
    sqlx::query_as(SELECT_MEDIA)
        .bind(Option::<i32>::None)
        .bind(query)
        .fetch_all(pool)
        .await
}

pub async fn get_media(pool: &PgPool, id: i32) -> Result<Option<Media>, sqlx::Error> {
    sqlx::query_as(SELECT_MEDIA)
        .bind(id)
        .bind(Option::<&str>::None)
        .fetch_optional(pool)
        .await
}

pub async fn get_identity(pool: &PgPool, id: i32) -> Result<Option<MediaIdentity>, sqlx::Error> {
    sqlx::query_as("SELECT tmdb_id, media_type_id AS media_type FROM media WHERE entity_id = $1")
        .bind(id)
        .fetch_optional(pool)
        .await
}

pub struct SavedMedia {
    pub id: i32,
    pub created: bool,
}

pub async fn save_media(
    transaction: &mut Transaction<'_, Postgres>,
    metadata: &MediaMetadata,
    existing_id: Option<i32>,
) -> Result<Option<SavedMedia>, sqlx::Error> {
    let media_type_id = metadata.media_type as i32;
    let (id, created) = if let Some(id) = existing_id {
        (id, false)
    } else {
        // Serialize imports of the same TMDB media before creating its entity.
        sqlx::query("SELECT pg_advisory_xact_lock($1, $2)")
            .bind(media_type_id)
            .bind(metadata.tmdb_id)
            .execute(&mut **transaction)
            .await?;
        let existing = sqlx::query_scalar(
            "SELECT entity_id FROM media WHERE tmdb_id = $1 AND media_type_id = $2 FOR UPDATE",
        )
        .bind(metadata.tmdb_id)
        .bind(media_type_id)
        .fetch_optional(&mut **transaction)
        .await?;

        if let Some(id) = existing {
            (id, false)
        } else {
            let id = entity_repository::create_entity(transaction, EntityType::Media).await?;
            sqlx::query(
                "INSERT INTO media (entity_id, tmdb_id, media_type_id) VALUES ($1, $2, $3)",
            )
            .bind(id)
            .bind(metadata.tmdb_id)
            .bind(media_type_id)
            .execute(&mut **transaction)
            .await?;
            (id, true)
        }
    };

    let updated = sqlx::query(
        "UPDATE media
         SET title = $2, original_title = $3, description = $4, poster_path = $5,
             backdrop_path = $6, release_date = $7, runtime_minutes = $8,
             production_status = $9, last_synced_at = NOW()
         WHERE entity_id = $1",
    )
    .bind(id)
    .bind(metadata.title.as_deref())
    .bind(metadata.original_title.as_deref())
    .bind(metadata.description.as_deref())
    .bind(metadata.poster_path.as_deref())
    .bind(metadata.backdrop_path.as_deref())
    .bind(metadata.release_date)
    .bind(metadata.runtime_minutes)
    .bind(metadata.production_status.as_deref())
    .execute(&mut **transaction)
    .await?;
    if updated.rows_affected() == 0 {
        return Ok(None);
    }

    if let Some(genres) = &metadata.genres {
        sqlx::query(
            "INSERT INTO media_genres (name)
             SELECT name FROM UNNEST($1::text[]) AS genre(name)
             ORDER BY name
             ON CONFLICT (name) DO NOTHING",
        )
        .bind(genres)
        .execute(&mut **transaction)
        .await?;
        sqlx::query("DELETE FROM media_genre_assignments WHERE media_id = $1")
            .bind(id)
            .execute(&mut **transaction)
            .await?;
        sqlx::query(
            "INSERT INTO media_genre_assignments (media_id, genre_id)
             SELECT $1, id FROM media_genres WHERE name = ANY($2::text[])",
        )
        .bind(id)
        .bind(genres)
        .execute(&mut **transaction)
        .await?;
    }
    for season in &metadata.seasons {
        save_season(transaction, id, season).await?;
    }

    Ok(Some(SavedMedia { id, created }))
}

async fn save_season(
    transaction: &mut Transaction<'_, Postgres>,
    media_id: i32,
    season: &SeasonMetadata,
) -> Result<(), sqlx::Error> {
    let season_id: i32 = sqlx::query_scalar(
        "INSERT INTO media_seasons
             (media_id, tmdb_id, season_number, title, description, poster_path, release_date)
         VALUES ($1, $2, $3, $4, $5, $6, $7)
         ON CONFLICT (tmdb_id) DO UPDATE SET
             media_id = EXCLUDED.media_id, season_number = EXCLUDED.season_number,
             title = EXCLUDED.title, description = EXCLUDED.description,
             poster_path = EXCLUDED.poster_path, release_date = EXCLUDED.release_date
         RETURNING id",
    )
    .bind(media_id)
    .bind(season.tmdb_id)
    .bind(season.season_number)
    .bind(season.title.as_deref())
    .bind(season.description.as_deref())
    .bind(season.poster_path.as_deref())
    .bind(season.release_date)
    .fetch_one(&mut **transaction)
    .await?;

    for episodes in season.episodes.chunks(1000) {
        let mut query = QueryBuilder::<Postgres>::new(
            "INSERT INTO media_episodes
                 (season_id, tmdb_id, episode_number, title, description, runtime_minutes, release_date) ",
        );
        query.push_values(episodes, |mut row, episode| {
            row.push_bind(season_id)
                .push_bind(episode.tmdb_id)
                .push_bind(episode.episode_number)
                .push_bind(episode.title.as_deref())
                .push_bind(episode.description.as_deref())
                .push_bind(episode.runtime_minutes)
                .push_bind(episode.release_date);
        });
        query.push(
            " ON CONFLICT (tmdb_id) DO UPDATE SET
                 season_id = EXCLUDED.season_id, episode_number = EXCLUDED.episode_number,
                 title = EXCLUDED.title, description = EXCLUDED.description,
                 runtime_minutes = EXCLUDED.runtime_minutes, release_date = EXCLUDED.release_date",
        );
        query.build().execute(&mut **transaction).await?;
    }

    Ok(())
}

pub async fn update_media(
    pool: &PgPool,
    id: i32,
    update: &UpdateMediaRequest,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE media
         SET personal_rating = CASE WHEN $2 THEN $3 ELSE personal_rating END,
             personal_notes = CASE WHEN $4 THEN $5 ELSE personal_notes END,
             completed_at = CASE
                 WHEN $6::integer IS NULL THEN completed_at
                 WHEN $6 <> $8 THEN NULL
                 WHEN watch_status_id = $8 THEN completed_at
                 ELSE NOW()
             END,
             watch_status_id = COALESCE($6, watch_status_id),
             watch_count = COALESCE($7, watch_count)
         WHERE entity_id = $1",
    )
    .bind(id)
    .bind(update.personal_rating.is_some())
    .bind(update.personal_rating.flatten())
    .bind(update.personal_notes.is_some())
    .bind(
        update
            .personal_notes
            .as_ref()
            .and_then(|notes| notes.as_deref()),
    )
    .bind(update.watch_status_id)
    .bind(update.watch_count)
    .bind(COMPLETED_WATCH_STATUS)
    .execute(pool)
    .await?;
    Ok(result.rows_affected() > 0)
}

pub async fn delete_media(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("DELETE FROM entities WHERE id = $1 AND type_id = $2")
        .bind(id)
        .bind(EntityType::Media as i32)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() > 0)
}

pub async fn get_seasons(
    pool: &PgPool,
    media_id: i32,
    season_id: Option<i32>,
) -> Result<Vec<MediaSeason>, sqlx::Error> {
    sqlx::query_as(
        "SELECT season.*, COUNT(episode.id) AS episode_count,
                COUNT(episode.id) FILTER (WHERE episode.watch_status_id = $3)
                    AS completed_episode_count
         FROM media_seasons season
         LEFT JOIN media_episodes episode ON episode.season_id = season.id
         WHERE season.media_id = $1 AND ($2::integer IS NULL OR season.id = $2)
         GROUP BY season.id
         ORDER BY season.season_number, season.id",
    )
    .bind(media_id)
    .bind(season_id)
    .bind(COMPLETED_WATCH_STATUS)
    .fetch_all(pool)
    .await
}

pub async fn get_episodes(pool: &PgPool, season_id: i32) -> Result<Vec<MediaEpisode>, sqlx::Error> {
    sqlx::query_as("SELECT * FROM media_episodes WHERE season_id = $1 ORDER BY episode_number, id")
        .bind(season_id)
        .fetch_all(pool)
        .await
}

pub async fn update_episode_status(
    pool: &PgPool,
    media_id: i32,
    season_id: i32,
    episode_id: i32,
    watch_status_id: i32,
) -> Result<bool, sqlx::Error> {
    let result = sqlx::query(
        "UPDATE media_episodes episode
         SET watched_at = CASE
                 WHEN $4::integer <> $5 THEN NULL
                 WHEN episode.watch_status_id = $5 THEN episode.watched_at
                 ELSE NOW()
             END,
             watch_status_id = $4
         FROM media_seasons season
         WHERE episode.id = $3 AND episode.season_id = season.id
           AND season.id = $2 AND season.media_id = $1",
    )
    .bind(media_id)
    .bind(season_id)
    .bind(episode_id)
    .bind(watch_status_id)
    .bind(COMPLETED_WATCH_STATUS)
    .execute(pool)
    .await?;
    Ok(result.rows_affected() > 0)
}
