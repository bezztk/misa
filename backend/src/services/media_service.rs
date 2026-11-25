use crate::api::error::{ApiError, ApiResult};
use crate::models::media::{MediaIdentity, MediaMetadata};
use crate::repositories::media_repository::{self, SavedMedia};
use crate::services::tmdb_client::{TmdbClient, TmdbError, image_url};
use sqlx::PgPool;
use std::io;
use std::path::{Path, PathBuf};

pub async fn sync_media(
    pool: &PgPool,
    identity: &MediaIdentity,
    existing_id: Option<i32>,
) -> ApiResult<SavedMedia> {
    let client = TmdbClient::new()?;
    let metadata = client.get_media(identity).await?;
    let tmdb_id = if existing_id.is_some() {
        identity.tmdb_id
    } else {
        metadata.tmdb_id
    };
    let images = download_images(&client, &metadata, tmdb_id).await?;

    let mut transaction = pool.begin().await?;
    let saved = media_repository::save_media(&mut transaction, &metadata, existing_id)
        .await?
        .ok_or(ApiError::NotFound)?;
    store_images(images)
        .await
        .map_err(|error| ApiError::Internal(Box::new(error)))?;
    transaction.commit().await?;
    Ok(saved)
}

struct DownloadedImage {
    path: PathBuf,
    bytes: Vec<u8>,
}

async fn download_images(
    client: &TmdbClient,
    metadata: &MediaMetadata,
    tmdb_id: i32,
) -> Result<Vec<DownloadedImage>, TmdbError> {
    let directory = PathBuf::from(metadata.media_type.as_str()).join(tmdb_id.to_string());
    let poster_url = image_url(metadata.poster_path.as_deref());
    let backdrop_url = image_url(metadata.backdrop_path.as_deref());
    let mut images = Vec::new();
    images.extend(download_image(client, &directory, "poster", poster_url.as_deref()).await?);
    images.extend(download_image(client, &directory, "backdrop", backdrop_url.as_deref()).await?);
    for season in &metadata.seasons {
        let season_directory = directory
            .join("seasons")
            .join(season.season_number.to_string());
        let url = image_url(season.poster_path.as_deref());
        images.extend(download_image(client, &season_directory, "poster", url.as_deref()).await?);

        let episodes_directory = season_directory.join("episodes");
        for episode in &season.episodes {
            let episode_directory = episodes_directory.join(episode.episode_number.to_string());
            let url = image_url(episode.still_path.as_deref());
            images
                .extend(download_image(client, &episode_directory, "still", url.as_deref()).await?);
        }
    }
    Ok(images)
}

async fn download_image(
    client: &TmdbClient,
    directory: &Path,
    name: &str,
    url: Option<&str>,
) -> Result<Option<DownloadedImage>, TmdbError> {
    let Some(url) = url else {
        return Ok(None);
    };
    let (_, extension) = url
        .rsplit_once('.')
        .expect("TMDB image URL has an extension");
    let path = directory.join(name).with_extension(extension);
    let bytes = client.download_image(url).await?;
    Ok(Some(DownloadedImage { path, bytes }))
}

async fn store_images(images: Vec<DownloadedImage>) -> io::Result<()> {
    if images.is_empty() {
        return Ok(());
    }
    let app_data =
        std::env::var_os("LOCALAPPDATA").ok_or_else(|| io::Error::other("LOCALAPPDATA missing"))?;
    let root = PathBuf::from(app_data).join("misa").join("media");

    for image in images {
        let path = root.join(image.path);
        let directory = path.parent().expect("TMDB image has a parent directory");
        tokio::fs::create_dir_all(directory).await?;
        let temporary_path = path.with_added_extension("tmp");
        tokio::fs::write(&temporary_path, image.bytes).await?;
        tokio::fs::rename(temporary_path, &path).await?;

        let name = path.file_name().expect("TMDB image has a file name");
        let mut files = tokio::fs::read_dir(directory).await?;
        while let Some(file) = files.next_entry().await? {
            let previous = file.path();
            if previous.file_stem() == path.file_stem()
                && !file.file_name().eq_ignore_ascii_case(name)
                && file.file_type().await?.is_file()
            {
                tokio::fs::remove_file(previous).await?;
            }
        }
    }
    Ok(())
}
