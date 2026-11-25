use crate::services::flashcard_review_service::FlashcardServiceError;
use crate::services::tmdb_client::TmdbError;
use axum::http::StatusCode;
use axum::response::{IntoResponse, Response};

#[derive(Debug)]
pub enum ApiError {
    BadRequest,
    NotFound,
    Conflict,
    BadGateway,
    Internal(Box<dyn std::error::Error + Send + Sync>),
}

pub type ApiResult<T> = Result<T, ApiError>;

impl IntoResponse for ApiError {
    fn into_response(self) -> Response {
        let status = match &self {
            Self::BadRequest => StatusCode::BAD_REQUEST,
            Self::NotFound => StatusCode::NOT_FOUND,
            Self::Conflict => StatusCode::CONFLICT,
            Self::BadGateway => StatusCode::BAD_GATEWAY,
            Self::Internal(error) => {
                eprintln!("Request failed: {error}");
                StatusCode::INTERNAL_SERVER_ERROR
            }
        };

        status.into_response()
    }
}

impl From<sqlx::Error> for ApiError {
    fn from(error: sqlx::Error) -> Self {
        Self::Internal(Box::new(error))
    }
}

impl From<fsrs::FSRSError> for ApiError {
    fn from(error: fsrs::FSRSError) -> Self {
        Self::Internal(Box::new(error))
    }
}

impl From<FlashcardServiceError> for ApiError {
    fn from(error: FlashcardServiceError) -> Self {
        match error {
            FlashcardServiceError::NotFound => Self::NotFound,
            FlashcardServiceError::Database(error) => error.into(),
            FlashcardServiceError::Calculation(error) => error.into(),
        }
    }
}

impl From<TmdbError> for ApiError {
    fn from(error: TmdbError) -> Self {
        match error {
            TmdbError::NotFound => Self::NotFound,
            TmdbError::Upstream => Self::BadGateway,
            TmdbError::MissingCredentials => {
                Self::Internal(Box::new(std::io::Error::other("TMDB credentials missing")))
            }
        }
    }
}
