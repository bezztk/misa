use crate::services::flashcard_review_service::FlashcardServiceError;
use axum::http::StatusCode;
use axum::response::{IntoResponse, Response};

#[derive(Debug)]
pub enum ApiError {
    BadRequest,
    NotFound,
    Conflict,
    Internal(Box<dyn std::error::Error + Send + Sync>),
}

pub type ApiResult<T> = Result<T, ApiError>;

impl IntoResponse for ApiError {
    fn into_response(self) -> Response {
        let status = match &self {
            Self::BadRequest => StatusCode::BAD_REQUEST,
            Self::NotFound => StatusCode::NOT_FOUND,
            Self::Conflict => StatusCode::CONFLICT,
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
