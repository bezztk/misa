use chrono::{DateTime, NaiveDate, Utc};
use serde::{Deserialize, Deserializer, Serialize};

pub const COMPLETED_WATCH_STATUS: i32 = 3;

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Deserialize, sqlx::Type)]
#[serde(rename_all = "lowercase")]
pub enum MediaType {
    Movie = 1,
    Tv = 2,
}

impl MediaType {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Movie => "movie",
            Self::Tv => "tv",
        }
    }
}

#[repr(i32)]
#[derive(
    Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Deserialize, Serialize, sqlx::Type,
)]
#[serde(rename_all = "snake_case")]
pub enum MediaRating {
    Disappointing = 1,
    Average = 2,
    WorthWatching = 3,
    Outstanding = 4,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct Media {
    pub entity_id: i32,
    pub tmdb_id: i32,
    pub media_type_id: i32,
    pub media_type: String,
    pub title: Option<String>,
    pub original_title: Option<String>,
    pub description: Option<String>,
    pub poster_path: Option<String>,
    pub backdrop_path: Option<String>,
    #[sqlx(skip)]
    pub poster_url: Option<String>,
    #[sqlx(skip)]
    pub backdrop_url: Option<String>,
    pub release_date: Option<NaiveDate>,
    pub runtime_minutes: Option<i32>,
    pub production_status: Option<String>,
    pub personal_rating: Option<MediaRating>,
    pub personal_notes: Option<String>,
    pub watch_status_id: i32,
    pub watch_count: i32,
    pub completed_at: Option<DateTime<Utc>>,
    pub last_synced_at: DateTime<Utc>,
    pub genres: Vec<String>,
    pub season_count: i64,
    pub episode_count: i64,
}

#[derive(Debug, sqlx::FromRow)]
pub struct MediaIdentity {
    pub tmdb_id: i32,
    pub media_type: MediaType,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct MediaSeason {
    pub id: i32,
    pub media_id: i32,
    pub tmdb_id: i32,
    pub season_number: i32,
    pub title: Option<String>,
    pub description: Option<String>,
    pub poster_path: Option<String>,
    #[sqlx(skip)]
    pub poster_url: Option<String>,
    pub release_date: Option<NaiveDate>,
    pub episode_count: i64,
    pub completed_episode_count: i64,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct MediaEpisode {
    pub id: i32,
    pub season_id: i32,
    pub tmdb_id: i32,
    pub episode_number: i32,
    pub title: Option<String>,
    pub description: Option<String>,
    pub runtime_minutes: Option<i32>,
    pub release_date: Option<NaiveDate>,
    pub watch_status_id: i32,
    pub watched_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Serialize)]
pub struct EpisodeProgress {
    pub episode_count: i64,
    pub completed_episode_count: i64,
    pub percentage: f64,
}

impl EpisodeProgress {
    pub fn new(episode_count: i64, completed_episode_count: i64) -> Self {
        Self {
            episode_count,
            completed_episode_count,
            percentage: if episode_count == 0 {
                0.0
            } else {
                completed_episode_count as f64 * 100.0 / episode_count as f64
            },
        }
    }
}

#[derive(Debug, Serialize)]
pub struct SeriesProgress {
    pub regular: EpisodeProgress,
    pub specials: EpisodeProgress,
}

#[derive(Debug, Deserialize)]
pub struct MediaQuery {
    pub query: Option<String>,
}

#[derive(Debug, Deserialize)]
#[serde(untagged, deny_unknown_fields)]
pub enum ImportMediaRequest {
    TmdbId { tmdb_id: i32, media_type: MediaType },
    TmdbUrl { url: String },
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct UpdateMediaRequest {
    #[serde(default, deserialize_with = "present_value")]
    pub personal_rating: Option<Option<MediaRating>>,
    #[serde(default, deserialize_with = "present_value")]
    pub personal_notes: Option<Option<String>>,
    #[serde(default, deserialize_with = "present_value")]
    pub watch_status_id: Option<i32>,
    #[serde(default, deserialize_with = "present_value")]
    pub watch_count: Option<i32>,
}

fn present_value<'de, D, T>(deserializer: D) -> Result<Option<T>, D::Error>
where
    D: Deserializer<'de>,
    T: Deserialize<'de>,
{
    T::deserialize(deserializer).map(Some)
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct UpdateEpisodeRequest {
    pub watch_status_id: i32,
}

pub struct MediaMetadata {
    pub tmdb_id: i32,
    pub media_type: MediaType,
    pub title: Option<String>,
    pub original_title: Option<String>,
    pub description: Option<String>,
    pub poster_path: Option<String>,
    pub backdrop_path: Option<String>,
    pub release_date: Option<NaiveDate>,
    pub runtime_minutes: Option<i32>,
    pub production_status: Option<String>,
    pub genres: Option<Vec<String>>,
    pub seasons: Vec<SeasonMetadata>,
}

pub struct SeasonMetadata {
    pub tmdb_id: i32,
    pub season_number: i32,
    pub title: Option<String>,
    pub description: Option<String>,
    pub poster_path: Option<String>,
    pub release_date: Option<NaiveDate>,
    pub episodes: Vec<EpisodeMetadata>,
}

pub struct EpisodeMetadata {
    pub tmdb_id: i32,
    pub episode_number: i32,
    pub title: Option<String>,
    pub description: Option<String>,
    pub runtime_minutes: Option<i32>,
    pub release_date: Option<NaiveDate>,
}
