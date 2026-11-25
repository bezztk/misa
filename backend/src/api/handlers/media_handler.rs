use crate::api::error::{ApiError, ApiResult};
use crate::api::response::IdResponse;
use crate::models::media::{
    EpisodeProgress, ImportMediaRequest, Media, MediaEpisode, MediaIdentity, MediaQuery,
    MediaSeason, MediaType, SeriesProgress, UpdateEpisodeRequest, UpdateMediaRequest,
};
use crate::repositories::media_repository;
use crate::services::{media_service, tmdb_client};
use axum::Json;
use axum::extract::{Path, Query, State};
use axum::http::StatusCode;
use sqlx::PgPool;

pub async fn get_all(
    State(pool): State<PgPool>,
    Query(query): Query<MediaQuery>,
) -> ApiResult<Json<Vec<Media>>> {
    let query = query.query.as_deref().map(str::trim);
    let mut media =
        media_repository::get_all(&pool, query.filter(|query| !query.is_empty())).await?;
    for item in &mut media {
        set_image_urls(item);
    }
    Ok(Json(media))
}

pub async fn get_media(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<Json<Media>> {
    let mut media = media_repository::get_media(&pool, id)
        .await?
        .ok_or(ApiError::NotFound)?;
    set_image_urls(&mut media);
    Ok(Json(media))
}

fn set_image_urls(media: &mut Media) {
    media.poster_url = tmdb_client::image_url(media.poster_path.as_deref());
    media.backdrop_url = tmdb_client::image_url(media.backdrop_path.as_deref());
}

pub async fn update_media(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
    Json(payload): Json<UpdateMediaRequest>,
) -> ApiResult<StatusCode> {
    if payload.watch_count.is_some_and(|count| count < 0)
        || payload
            .watch_status_id
            .is_some_and(|status| !valid_watch_status(status))
    {
        return Err(ApiError::BadRequest);
    }
    if !media_repository::update_media(&pool, id, &payload).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn delete_media(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<StatusCode> {
    if !media_repository::delete_media(&pool, id).await? {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn import_media(
    State(pool): State<PgPool>,
    Json(payload): Json<ImportMediaRequest>,
) -> ApiResult<(StatusCode, Json<IdResponse>)> {
    let identity = match payload {
        ImportMediaRequest::TmdbId {
            tmdb_id,
            media_type,
        } if tmdb_id > 0 => MediaIdentity {
            tmdb_id,
            media_type,
        },
        ImportMediaRequest::TmdbUrl { url } => {
            tmdb_client::parse_media_url(&url).ok_or(ApiError::BadRequest)?
        }
        _ => return Err(ApiError::BadRequest),
    };

    let saved = media_service::sync_media(&pool, &identity, None).await?;
    let status = if saved.created {
        StatusCode::CREATED
    } else {
        StatusCode::OK
    };
    Ok((status, Json(IdResponse { id: saved.id })))
}

pub async fn sync_media(State(pool): State<PgPool>, Path(id): Path<i32>) -> ApiResult<StatusCode> {
    let identity = media_repository::get_identity(&pool, id)
        .await?
        .ok_or(ApiError::NotFound)?;
    media_service::sync_media(&pool, &identity, Some(id)).await?;
    Ok(StatusCode::NO_CONTENT)
}

pub async fn get_seasons(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<Json<Vec<MediaSeason>>> {
    let mut seasons = get_series_seasons(&pool, id).await?;
    for season in &mut seasons {
        season.poster_url = tmdb_client::image_url(season.poster_path.as_deref());
    }
    Ok(Json(seasons))
}

pub async fn get_episodes(
    State(pool): State<PgPool>,
    Path((media_id, season_id)): Path<(i32, i32)>,
) -> ApiResult<Json<Vec<MediaEpisode>>> {
    get_season(&pool, media_id, season_id).await?;
    Ok(Json(
        media_repository::get_episodes(&pool, season_id).await?,
    ))
}

pub async fn update_episode_status(
    State(pool): State<PgPool>,
    Path((media_id, season_id, episode_id)): Path<(i32, i32, i32)>,
    Json(payload): Json<UpdateEpisodeRequest>,
) -> ApiResult<StatusCode> {
    if !valid_watch_status(payload.watch_status_id) {
        return Err(ApiError::BadRequest);
    }
    if !media_repository::update_episode_status(
        &pool,
        media_id,
        season_id,
        episode_id,
        payload.watch_status_id,
    )
    .await?
    {
        return Err(ApiError::NotFound);
    }
    Ok(StatusCode::NO_CONTENT)
}

pub async fn get_series_progress(
    State(pool): State<PgPool>,
    Path(id): Path<i32>,
) -> ApiResult<Json<SeriesProgress>> {
    let mut regular = (0, 0);
    let mut specials = (0, 0);
    for season in get_series_seasons(&pool, id).await? {
        let (total, completed) = if season.season_number == 0 {
            &mut specials
        } else {
            &mut regular
        };
        *total += season.episode_count;
        *completed += season.completed_episode_count;
    }
    Ok(Json(SeriesProgress {
        regular: EpisodeProgress::new(regular.0, regular.1),
        specials: EpisodeProgress::new(specials.0, specials.1),
    }))
}

pub async fn get_season_progress(
    State(pool): State<PgPool>,
    Path((media_id, season_id)): Path<(i32, i32)>,
) -> ApiResult<Json<EpisodeProgress>> {
    let season = get_season(&pool, media_id, season_id).await?;
    Ok(Json(EpisodeProgress::new(
        season.episode_count,
        season.completed_episode_count,
    )))
}

async fn get_series_seasons(pool: &PgPool, id: i32) -> ApiResult<Vec<MediaSeason>> {
    let identity = media_repository::get_identity(pool, id)
        .await?
        .ok_or(ApiError::NotFound)?;
    if identity.media_type != MediaType::Tv {
        return Err(ApiError::NotFound);
    }
    Ok(media_repository::get_seasons(pool, id, None).await?)
}

async fn get_season(pool: &PgPool, media_id: i32, season_id: i32) -> ApiResult<MediaSeason> {
    media_repository::get_seasons(pool, media_id, Some(season_id))
        .await?
        .pop()
        .ok_or(ApiError::NotFound)
}

fn valid_watch_status(status: i32) -> bool {
    (1..=5).contains(&status)
}
