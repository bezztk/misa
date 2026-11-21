use std::fmt::{Display, Formatter};
use std::time::Duration;

use feed_rs::model::Entry;
use reqwest::Client;
use sqlx::PgPool;

use crate::models::rss::RssFeed;
use crate::repositories::rss_repository::{self, FetchedEntry};

const MAX_FEED_SIZE: usize = 5 * 1024 * 1024;
const FETCH_INTERVAL: Duration = Duration::from_secs(30 * 60);

#[derive(Debug)]
pub enum RssFetchError {
    Http(reqwest::Error),
    Parse(feed_rs::parser::ParseFeedError),
    Database(sqlx::Error),
    TooLarge,
}

impl Display for RssFetchError {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Http(error) => write!(formatter, "HTTP request failed: {error}"),
            Self::Parse(error) => write!(formatter, "Feed could not be parsed: {error}"),
            Self::Database(error) => write!(formatter, "Feed could not be saved: {error}"),
            Self::TooLarge => write!(formatter, "Feed exceeds the 5 MiB size limit"),
        }
    }
}

impl std::error::Error for RssFetchError {}

pub fn create_http_client() -> Result<Client, reqwest::Error> {
    Client::builder()
        .connect_timeout(Duration::from_secs(5))
        .timeout(Duration::from_secs(20))
        .user_agent("Misa RSS/0.2")
        .build()
}

pub async fn fetch_feed(
    pool: &PgPool,
    client: &Client,
    feed: &RssFeed,
) -> Result<(), RssFetchError> {
    let mut response = client
        .get(&feed.url)
        .send()
        .await
        .map_err(RssFetchError::Http)?
        .error_for_status()
        .map_err(RssFetchError::Http)?;

    if response
        .content_length()
        .is_some_and(|size| size > MAX_FEED_SIZE as u64)
    {
        return Err(RssFetchError::TooLarge);
    }

    let mut body = Vec::new();
    while let Some(chunk) = response.chunk().await.map_err(RssFetchError::Http)? {
        if body.len() + chunk.len() > MAX_FEED_SIZE {
            return Err(RssFetchError::TooLarge);
        }
        body.extend_from_slice(&chunk);
    }

    let parsed = feed_rs::parser::parse(body.as_slice()).map_err(RssFetchError::Parse)?;
    let title = parsed
        .title
        .as_ref()
        .map(|title| title.content.trim())
        .filter(|title| !title.is_empty())
        .unwrap_or(&feed.url);
    let entries = parsed.entries.iter().map(map_entry).collect::<Vec<_>>();

    rss_repository::save_fetched_feed(pool, feed.id, title, &entries)
        .await
        .map_err(RssFetchError::Database)
}

fn map_entry(entry: &Entry) -> FetchedEntry {
    let url = entry
        .links
        .iter()
        .find(|link| link.rel.as_deref().is_none_or(|rel| rel == "alternate"))
        .or_else(|| entry.links.first())
        .map(|link| link.href.clone());
    let title = entry
        .title
        .as_ref()
        .map(|title| title.content.trim().to_string())
        .filter(|title| !title.is_empty())
        .unwrap_or_else(|| "Ohne Titel".to_string());
    let summary = entry
        .summary
        .as_ref()
        .map(|summary| summary.content.clone())
        .or_else(|| {
            entry
                .content
                .as_ref()
                .and_then(|content| content.body.clone())
        });
    let external_id = if url.is_some() {
        entry.id.clone()
    } else {
        format!(
            "{}|{}",
            title,
            entry
                .published
                .or(entry.updated)
                .map(|date| date.to_rfc3339())
                .unwrap_or_default()
        )
    };

    FetchedEntry {
        external_id,
        title,
        url,
        summary,
        published_at: entry.published.or(entry.updated),
    }
}

pub fn start_fetch_loop(pool: PgPool, client: Client) {
    tokio::spawn(async move {
        let mut interval = tokio::time::interval(FETCH_INTERVAL);
        loop {
            interval.tick().await;
            let feeds = match rss_repository::get_active_feeds(&pool).await {
                Ok(feeds) => feeds,
                Err(error) => {
                    eprintln!("RSS feeds could not be loaded: {error}");
                    continue;
                }
            };
            for feed in feeds {
                if let Err(error) = fetch_feed(&pool, &client, &feed).await {
                    eprintln!("RSS feed {} failed: {error}", feed.url);
                }
            }
        }
    });
}
