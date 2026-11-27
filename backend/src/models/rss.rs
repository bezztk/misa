use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, sqlx::FromRow, Serialize)]
pub struct RssFeed {
    pub id: i32,
    pub url: String,
    pub title: String,
    pub last_fetched_at: Option<DateTime<Utc>>,
    pub enabled: bool,
    pub is_hidden: bool,
}

#[derive(Debug, Clone, sqlx::FromRow, Serialize)]
pub struct RssEntry {
    pub id: i32,
    pub feed_id: i32,
    pub feed_title: String,
    pub external_id: String,
    pub title: String,
    pub url: Option<String>,
    pub summary: Option<String>,
    pub published_at: Option<DateTime<Utc>>,
    pub is_read: bool,
}

#[derive(Debug, Deserialize)]
pub struct CreateRssFeedRequest {
    pub url: String,
}

#[derive(Debug, Deserialize)]
pub struct UpdateRssFeedRequest {
    pub url: String,
    pub enabled: bool,
}

#[derive(Debug, Deserialize)]
pub struct UpdateRssEntryRequest {
    pub is_read: bool,
}

#[derive(Debug, Deserialize)]
pub struct RssEntriesQuery {
    pub feed_id: Option<i32>,
}
