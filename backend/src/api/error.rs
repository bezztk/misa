use crate::repositories::flashcard_repository::ReviewFlashcardError;
use axum::http::StatusCode;
use axum::response::{IntoResponse, Response};

#[derive(Debug)]
pub enum ApiError {
    BadRequest,
    NotFound,
    Internal(Box<dyn std::error::Error + Send + Sync>),
}

pub type ApiResult<T> = Result<T, ApiError>;

impl IntoResponse for ApiError {
    fn into_response(self) -> Response {
        let status = match &self {
            Self::BadRequest => StatusCode::BAD_REQUEST,
            Self::NotFound => StatusCode::NOT_FOUND,
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

impl From<ReviewFlashcardError> for ApiError {
    fn from(error: ReviewFlashcardError) -> Self {
        match error {
            ReviewFlashcardError::NotFound => Self::NotFound,
            ReviewFlashcardError::Database(error) => error.into(),
            ReviewFlashcardError::Calculation(error) => error.into(),
        }
    }
}
