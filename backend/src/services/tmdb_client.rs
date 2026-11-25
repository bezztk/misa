use crate::models::media::{
    EpisodeMetadata, MAX_ADDITIONAL_BACKDROPS, MediaIdentity, MediaMetadata, MediaType,
    SeasonMetadata,
};
use chrono::NaiveDate;
use reqwest::{Client, StatusCode, Url};
use serde::{Deserialize, de::DeserializeOwned};
use std::time::Duration;

const API_URL: &str = "https://api.themoviedb.org/3";
const IMAGE_URL: &str = "https://image.tmdb.org/t/p/original";

#[derive(Debug)]
pub enum TmdbError {
    MissingCredentials,
    NotFound,
    Upstream,
}

enum Credentials {
    ReadAccessToken(String),
    ApiKey(String),
}

pub struct TmdbClient {
    http: Client,
    credentials: Credentials,
}

impl TmdbClient {
    pub fn new() -> Result<Self, TmdbError> {
        let credentials = match env_credential("TMDB_API_READ_ACCESS_TOKEN") {
            Some(token) => Credentials::ReadAccessToken(token),
            None => Credentials::ApiKey(
                env_credential("TMDB_API_KEY").ok_or(TmdbError::MissingCredentials)?,
            ),
        };
        let http = Client::builder()
            .connect_timeout(Duration::from_secs(5))
            .timeout(Duration::from_secs(20))
            .build()
            .map_err(|_| TmdbError::Upstream)?;
        Ok(Self { http, credentials })
    }

    pub async fn get_media(&self, identity: &MediaIdentity) -> Result<MediaMetadata, TmdbError> {
        let MediaIdentity {
            tmdb_id,
            media_type,
        } = *identity;
        let path = format!("{}/{tmdb_id}", media_type.as_str());
        let response: MediaResponse = self
            .get(
                &path,
                &[
                    ("append_to_response", "videos,keywords,images"),
                    ("include_image_language", "en,null"),
                ],
            )
            .await?;
        let mut seasons = Vec::new();

        if media_type == MediaType::Tv {
            for season in response.seasons.unwrap_or_default() {
                let season_number = season.season_number;
                let details: Result<SeasonResponse, _> = self
                    .get(&format!("{path}/season/{season_number}"), &[])
                    .await;
                match details {
                    Ok(details) => seasons.push(details.into_metadata()),
                    Err(TmdbError::NotFound) => continue,
                    Err(error) => return Err(error),
                }
            }
        }

        let (title, original_title, release_date) = match media_type {
            MediaType::Movie => (
                response.title,
                response.original_title,
                response.release_date,
            ),
            MediaType::Tv => (
                response.name,
                response.original_name,
                response.first_air_date,
            ),
        };
        let backdrop_path = nonempty(response.backdrop_path);
        let additional_backdrop_paths =
            select_additional_backdrops(response.images, backdrop_path.as_deref());
        Ok(MediaMetadata {
            tmdb_id: response.id,
            media_type,
            title: nonempty(title),
            original_title: nonempty(original_title),
            description: nonempty(response.overview),
            poster_path: nonempty(response.poster_path),
            backdrop_path,
            trailer_url: select_trailer(response.videos),
            additional_backdrop_paths,
            release_date: parse_date(release_date),
            runtime_minutes: response
                .runtime
                .filter(|&runtime| media_type == MediaType::Movie && runtime > 0),
            production_status: nonempty(response.status),
            tmdb_vote_average: response.vote_average,
            tmdb_vote_count: response.vote_count,
            genres: response.genres.map(|genres| {
                genres
                    .into_iter()
                    .filter_map(|genre| nonempty(genre.name))
                    .collect()
            }),
            keywords: response.keywords.and_then(|keywords| {
                keywords.entries.map(|entries| {
                    entries
                        .into_iter()
                        .map(|entry| (entry.id, entry.name))
                        .collect()
                })
            }),
            seasons,
        })
    }

    pub async fn download_image(&self, url: &str) -> Result<Vec<u8>, TmdbError> {
        let response = self
            .http
            .get(url)
            .send()
            .await
            .and_then(|response| response.error_for_status())
            .map_err(|_| TmdbError::Upstream)?;
        let bytes = response.bytes().await.map_err(|_| TmdbError::Upstream)?;
        Ok(bytes.to_vec())
    }

    async fn get<T: DeserializeOwned>(
        &self,
        path: &str,
        query: &[(&str, &str)],
    ) -> Result<T, TmdbError> {
        let mut url = Url::parse(&format!("{API_URL}/{path}")).expect("valid TMDB endpoint");
        url.query_pairs_mut()
            .append_pair("language", "en-US")
            .extend_pairs(query.iter().copied());

        let request = match &self.credentials {
            Credentials::ReadAccessToken(token) => self.http.get(url).bearer_auth(token),
            Credentials::ApiKey(key) => {
                url.query_pairs_mut().append_pair("api_key", key);
                self.http.get(url)
            }
        };
        let response = request.send().await.map_err(|_| TmdbError::Upstream)?;
        if response.status() == StatusCode::NOT_FOUND {
            return Err(TmdbError::NotFound);
        }
        let response = response
            .error_for_status()
            .map_err(|_| TmdbError::Upstream)?;
        let bytes = response.bytes().await.map_err(|_| TmdbError::Upstream)?;
        serde_json::from_slice(&bytes).map_err(|_| TmdbError::Upstream)
    }
}

pub fn parse_media_url(input: &str) -> Option<MediaIdentity> {
    let url = Url::parse(input.trim()).ok()?;
    if !matches!(url.scheme(), "http" | "https")
        || !matches!(
            url.host_str(),
            Some("themoviedb.org" | "www.themoviedb.org")
        )
    {
        return None;
    }
    let mut segments = url.path_segments()?;
    let media_type = match segments.next()? {
        "movie" => MediaType::Movie,
        "tv" => MediaType::Tv,
        _ => return None,
    };
    let tmdb_id = segments.next()?.split('-').next()?.parse().ok()?;
    (tmdb_id > 0).then_some(MediaIdentity {
        tmdb_id,
        media_type,
    })
}

pub fn image_url(path: Option<&str>) -> Option<String> {
    let filename = path?.strip_prefix('/')?;
    let (stem, extension) = filename.rsplit_once('.')?;
    if stem.is_empty()
        || extension.is_empty()
        || !stem
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'_' | b'-' | b'.'))
        || !extension.bytes().all(|byte| byte.is_ascii_alphanumeric())
    {
        return None;
    }

    Some(format!("{IMAGE_URL}/{filename}"))
}

fn env_credential(name: &str) -> Option<String> {
    nonempty(std::env::var(name).ok()).map(|value| value.trim().to_owned())
}

fn nonempty(value: Option<String>) -> Option<String> {
    value.filter(|value| !value.trim().is_empty())
}

fn parse_date(date: Option<String>) -> Option<NaiveDate> {
    date.and_then(|date| NaiveDate::parse_from_str(&date, "%Y-%m-%d").ok())
}

fn select_trailer(videos: Option<VideosResponse>) -> Option<String> {
    videos?
        .results?
        .into_iter()
        .filter(|video| {
            video.kind.as_deref() == Some("Trailer")
                && video.site.as_deref() == Some("YouTube")
                && video.key.as_deref().is_some_and(|key| {
                    !key.is_empty()
                        && key
                            .bytes()
                            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'_' | b'-'))
                })
        })
        .min_by_key(|video| !video.official.unwrap_or(false))
        .and_then(|video| video.key)
        .map(|key| format!("https://www.youtube.com/watch?v={key}"))
}

fn select_additional_backdrops(
    images: Option<ImagesResponse>,
    primary: Option<&str>,
) -> Vec<String> {
    let mut backdrops = Vec::new();
    for path in images
        .and_then(|images| images.backdrops)
        .unwrap_or_default()
        .into_iter()
        .filter_map(|image| image.file_path)
    {
        if Some(path.as_str()) != primary
            && !backdrops.contains(&path)
            && image_url(Some(&path)).is_some()
        {
            backdrops.push(path);
            if backdrops.len() == MAX_ADDITIONAL_BACKDROPS {
                break;
            }
        }
    }
    backdrops
}

#[derive(Deserialize)]
struct MediaResponse {
    id: i32,
    title: Option<String>,
    original_title: Option<String>,
    name: Option<String>,
    original_name: Option<String>,
    overview: Option<String>,
    poster_path: Option<String>,
    backdrop_path: Option<String>,
    release_date: Option<String>,
    first_air_date: Option<String>,
    runtime: Option<i32>,
    status: Option<String>,
    vote_average: Option<f64>,
    vote_count: Option<i32>,
    genres: Option<Vec<GenreResponse>>,
    seasons: Option<Vec<SeasonSummary>>,
    videos: Option<VideosResponse>,
    keywords: Option<KeywordsResponse>,
    images: Option<ImagesResponse>,
}

#[derive(Deserialize)]
struct VideosResponse {
    results: Option<Vec<VideoResponse>>,
}

#[derive(Deserialize)]
struct VideoResponse {
    #[serde(rename = "type")]
    kind: Option<String>,
    site: Option<String>,
    key: Option<String>,
    official: Option<bool>,
}

#[derive(Deserialize)]
struct KeywordsResponse {
    #[serde(rename = "keywords", alias = "results")]
    entries: Option<Vec<KeywordResponse>>,
}

#[derive(Deserialize)]
struct KeywordResponse {
    id: i32,
    name: String,
}

#[derive(Deserialize)]
struct ImagesResponse {
    backdrops: Option<Vec<ImageResponse>>,
}

#[derive(Deserialize)]
struct ImageResponse {
    file_path: Option<String>,
}

#[derive(Deserialize)]
struct GenreResponse {
    name: Option<String>,
}

#[derive(Deserialize)]
struct SeasonSummary {
    season_number: i32,
}

#[derive(Deserialize)]
struct SeasonResponse {
    id: i32,
    season_number: i32,
    name: Option<String>,
    overview: Option<String>,
    poster_path: Option<String>,
    air_date: Option<String>,
    episodes: Option<Vec<EpisodeResponse>>,
}

impl SeasonResponse {
    fn into_metadata(self) -> SeasonMetadata {
        SeasonMetadata {
            tmdb_id: self.id,
            season_number: self.season_number,
            title: nonempty(self.name),
            description: nonempty(self.overview),
            poster_path: nonempty(self.poster_path),
            release_date: parse_date(self.air_date),
            episodes: self
                .episodes
                .unwrap_or_default()
                .into_iter()
                .map(|episode| EpisodeMetadata {
                    tmdb_id: episode.id,
                    episode_number: episode.episode_number,
                    title: nonempty(episode.name),
                    description: nonempty(episode.overview),
                    still_path: nonempty(episode.still_path),
                    runtime_minutes: episode.runtime.filter(|&runtime| runtime > 0),
                    release_date: parse_date(episode.air_date),
                })
                .collect(),
        }
    }
}

#[derive(Deserialize)]
struct EpisodeResponse {
    id: i32,
    episode_number: i32,
    name: Option<String>,
    overview: Option<String>,
    still_path: Option<String>,
    runtime: Option<i32>,
    air_date: Option<String>,
}
