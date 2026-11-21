use chrono::{DateTime, Utc};
use sqlx::{PgPool, Postgres, Transaction};

use crate::models::rss::{RssEntry, RssFeed};

pub async fn get_feeds(pool: &PgPool) -> Result<Vec<RssFeed>, sqlx::Error> {
    sqlx::query_as::<_, RssFeed>(
        "SELECT id, url, title, last_fetched_at, enabled, is_hidden
         FROM rss_feeds
         WHERE is_hidden = FALSE
         ORDER BY title, id",
    )
    .fetch_all(pool)
    .await
}

pub async fn get_active_feeds(pool: &PgPool) -> Result<Vec<RssFeed>, sqlx::Error> {
    sqlx::query_as::<_, RssFeed>(
        "SELECT id, url, title, last_fetched_at, enabled, is_hidden
         FROM rss_feeds
         WHERE enabled = TRUE AND is_hidden = FALSE
         ORDER BY id",
    )
    .fetch_all(pool)
    .await
}

pub async fn create_or_reactivate_feed(pool: &PgPool, url: &str) -> Result<RssFeed, sqlx::Error> {
    sqlx::query_as::<_, RssFeed>(
        "INSERT INTO rss_feeds (url, title)
         VALUES ($1, $1)
         ON CONFLICT (url) DO UPDATE SET is_hidden = FALSE
         RETURNING id, url, title, last_fetched_at, enabled, is_hidden",
    )
    .bind(url)
    .fetch_one(pool)
    .await
}

pub async fn update_feed_enabled(
    pool: &PgPool,
    id: i32,
    enabled: bool,
) -> Result<bool, sqlx::Error> {
    let result =
        sqlx::query("UPDATE rss_feeds SET enabled = $2 WHERE id = $1 AND is_hidden = FALSE")
            .bind(id)
            .bind(enabled)
            .execute(pool)
            .await?;
    Ok(result.rows_affected() > 0)
}

pub async fn hide_feed(pool: &PgPool, id: i32) -> Result<bool, sqlx::Error> {
    let result =
        sqlx::query("UPDATE rss_feeds SET is_hidden = TRUE WHERE id = $1 AND is_hidden = FALSE")
            .bind(id)
            .execute(pool)
            .await?;
    Ok(result.rows_affected() > 0)
}

pub async fn get_entries(
    pool: &PgPool,
    feed_id: Option<i32>,
) -> Result<Vec<RssEntry>, sqlx::Error> {
    sqlx::query_as::<_, RssEntry>(
        "SELECT entry.id, entry.feed_id, feed.title AS feed_title,
                entry.external_id, entry.title, entry.url, entry.summary,
                entry.published_at, entry.is_read
         FROM rss_entries entry
         JOIN rss_feeds feed ON feed.id = entry.feed_id
         WHERE feed.is_hidden = FALSE AND ($1::integer IS NULL OR entry.feed_id = $1)
         ORDER BY entry.published_at DESC NULLS LAST, entry.id DESC",
    )
    .bind(feed_id)
    .fetch_all(pool)
    .await
}

pub async fn update_entry_read(pool: &PgPool, id: i32, is_read: bool) -> Result<bool, sqlx::Error> {
    let result = sqlx::query("UPDATE rss_entries SET is_read = $2 WHERE id = $1")
        .bind(id)
        .bind(is_read)
        .execute(pool)
        .await?;
    Ok(result.rows_affected() > 0)
}

pub async fn save_fetched_feed(
    pool: &PgPool,
    feed_id: i32,
    title: &str,
    entries: &[FetchedEntry],
) -> Result<(), sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let can_update = sqlx::query_scalar::<_, bool>(
        "SELECT enabled AND NOT is_hidden
         FROM rss_feeds
         WHERE id = $1
         FOR UPDATE",
    )
    .bind(feed_id)
    .fetch_optional(&mut *transaction)
    .await?
    .unwrap_or(false);
    if !can_update {
        return Ok(());
    }

    for entry in entries {
        insert_entry(&mut transaction, feed_id, entry).await?;
    }
    sqlx::query(
        "UPDATE rss_feeds
         SET title = $2, last_fetched_at = NOW()
         WHERE id = $1",
    )
    .bind(feed_id)
    .bind(title)
    .execute(&mut *transaction)
    .await?;
    transaction.commit().await
}

async fn insert_entry(
    transaction: &mut Transaction<'_, Postgres>,
    feed_id: i32,
    entry: &FetchedEntry,
) -> Result<(), sqlx::Error> {
    sqlx::query(
        "INSERT INTO rss_entries
             (feed_id, external_id, title, url, summary, published_at)
         VALUES ($1, $2, $3, $4, $5, $6)
         ON CONFLICT (feed_id, external_id) DO NOTHING",
    )
    .bind(feed_id)
    .bind(&entry.external_id)
    .bind(&entry.title)
    .bind(&entry.url)
    .bind(&entry.summary)
    .bind(entry.published_at)
    .execute(&mut **transaction)
    .await?;
    Ok(())
}

pub struct FetchedEntry {
    pub external_id: String,
    pub title: String,
    pub url: Option<String>,
    pub summary: Option<String>,
    pub published_at: Option<DateTime<Utc>>,
}
