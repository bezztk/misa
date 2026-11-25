use chrono::{DateTime, Duration, Utc};
use fsrs::{FSRS, ItemState, MemoryState};
use sqlx::PgPool;

use crate::models::flashcard::{
    FlashcardQueueItem, FlashcardReviewPreview, FlashcardReviewStateId,
};
use crate::repositories::flashcard_repository::{self, NewFlashcardReview};

#[derive(Debug)]
pub enum FlashcardServiceError {
    NotFound,
    Database(sqlx::Error),
    Calculation(fsrs::FSRSError),
}

impl From<sqlx::Error> for FlashcardServiceError {
    fn from(error: sqlx::Error) -> Self {
        Self::Database(error)
    }
}

impl From<fsrs::FSRSError> for FlashcardServiceError {
    fn from(error: fsrs::FSRSError) -> Self {
        Self::Calculation(error)
    }
}

pub struct FlashcardReviewSchedule {
    pub review_state_id: i32,
    pub stability: f32,
    pub difficulty: f32,
    pub reviewed_at: DateTime<Utc>,
    pub next_due_at: DateTime<Utc>,
    pub interval_days: f32,
}

pub struct FlashcardReviewParameters {
    pub review_state_id: i32,
    pub last_rating: Option<i16>,
    pub stability: Option<f32>,
    pub difficulty: Option<f32>,
    pub last_reviewed_at: Option<DateTime<Utc>>,
    pub desired_retention: f32,
    pub reviewed_at: DateTime<Utc>,
}

const REVIEW_STATE_NEW: i32 = FlashcardReviewStateId::New as i32;
const REVIEW_STATE_LEARNING: i32 = FlashcardReviewStateId::Learning as i32;
const REVIEW_STATE_REVIEW: i32 = FlashcardReviewStateId::Review as i32;
const REVIEW_STATE_RELEARNING: i32 = FlashcardReviewStateId::Relearning as i32;
const MILLISECONDS_PER_DAY: f32 = 86_400_000.0;

pub async fn get_due_flashcards(
    pool: &PgPool,
) -> Result<Vec<FlashcardQueueItem>, FlashcardServiceError> {
    let contexts = flashcard_repository::get_due_flashcard_contexts(pool).await?;
    let entity_ids = contexts
        .iter()
        .map(|context| context.id)
        .collect::<Vec<_>>();
    let comments =
        crate::repositories::comment_repository::get_comments_for_entities(pool, &entity_ids)
            .await?;
    let reviewed_at = Utc::now();
    let mut flashcards = Vec::with_capacity(contexts.len());

    for context in contexts {
        let parameters = FlashcardReviewParameters {
            review_state_id: context.review_state_id,
            last_rating: context.last_rating,
            stability: context.stability,
            difficulty: context.difficulty,
            last_reviewed_at: context.last_reviewed_at,
            desired_retention: context.desired_retention,
            reviewed_at,
        };
        let preview = calculate_review_preview(&parameters)?;

        flashcards.push(FlashcardQueueItem {
            id: context.id,
            front: context.front,
            back: context.back,
            hint: context.hint,
            review_state_id: context.review_state_id,
            retention_class_id: context.retention_class_id,
            learning_step: i16::from(
                matches!(context.review_state_id, 2 | 4) && context.last_rating == Some(3),
            ),
            stability: context.stability,
            difficulty: context.difficulty,
            preview,
            comments: comments
                .iter()
                .filter(|comment| comment.entity_id == context.id)
                .cloned()
                .collect(),
        });
    }

    Ok(flashcards)
}

pub async fn review_flashcard(
    pool: &PgPool,
    id: i32,
    rating: i16,
    duration_ms: i32,
) -> Result<(), FlashcardServiceError> {
    let mut transaction = pool.begin().await?;
    let context = flashcard_repository::get_flashcard_review_context(&mut transaction, id)
        .await?
        .ok_or(FlashcardServiceError::NotFound)?;
    let parameters = FlashcardReviewParameters {
        review_state_id: context.review_state_id,
        last_rating: context.last_rating,
        stability: context.stability,
        difficulty: context.difficulty,
        last_reviewed_at: context.last_reviewed_at,
        desired_retention: context.desired_retention,
        reviewed_at: Utc::now(),
    };
    let schedule = calculate_review_schedule(&parameters, rating)?;

    flashcard_repository::save_flashcard_review(
        &mut transaction,
        &NewFlashcardReview {
            flashcard_id: id,
            rating,
            duration_ms,
            review_state_before_id: context.review_state_id,
            review_state_after_id: schedule.review_state_id,
            stability: schedule.stability,
            difficulty: schedule.difficulty,
            reviewed_at: schedule.reviewed_at,
            next_due_at: schedule.next_due_at,
        },
    )
    .await?;
    transaction.commit().await?;

    Ok(())
}

pub fn calculate_review_schedule(
    parameters: &FlashcardReviewParameters,
    rating: i16,
) -> fsrs::Result<FlashcardReviewSchedule> {
    let previous_memory =
        parameters
            .stability
            .zip(parameters.difficulty)
            .map(|(stability, difficulty)| MemoryState {
                stability,
                difficulty,
            });
    let elapsed_days = parameters
        .last_reviewed_at
        .map(|last_reviewed_at| {
            (parameters.reviewed_at - last_reviewed_at)
                .num_days()
                .max(0) as u32
        })
        .unwrap_or(0);
    let next_states =
        FSRS::default().next_states(previous_memory, parameters.desired_retention, elapsed_days)?;
    let next_memory: ItemState = match rating {
        1 => next_states.again,
        2 => next_states.hard,
        3 => next_states.good,
        4 => next_states.easy,
        _ => unreachable!("rating is validated by the handler"),
    };

    let next_review_state_id =
        next_review_state(parameters.review_state_id, parameters.last_rating, rating);
    let fsrs_duration = Duration::milliseconds(
        (next_memory.interval.max(1.0) * MILLISECONDS_PER_DAY)
            .round()
            .max(1.0) as i64,
    );
    let interval = if next_review_state_id == REVIEW_STATE_REVIEW {
        fsrs_duration
    } else {
        learning_interval(rating)
    };
    let interval_days = interval.num_milliseconds() as f32 / MILLISECONDS_PER_DAY;

    Ok(FlashcardReviewSchedule {
        review_state_id: next_review_state_id,
        stability: next_memory.memory.stability,
        difficulty: next_memory.memory.difficulty,
        reviewed_at: parameters.reviewed_at,
        next_due_at: parameters.reviewed_at + interval,
        interval_days,
    })
}

pub fn calculate_review_preview(
    parameters: &FlashcardReviewParameters,
) -> fsrs::Result<FlashcardReviewPreview> {
    let interval_for = |rating| {
        calculate_review_schedule(parameters, rating).map(|schedule| schedule.interval_days)
    };

    Ok(FlashcardReviewPreview {
        again: interval_for(1)?,
        hard: interval_for(2)?,
        good: interval_for(3)?,
        easy: interval_for(4)?,
    })
}

fn next_review_state(review_state_id: i32, last_rating: Option<i16>, rating: i16) -> i32 {
    match review_state_id {
        REVIEW_STATE_NEW => {
            if rating == 4 {
                REVIEW_STATE_REVIEW
            } else {
                REVIEW_STATE_LEARNING
            }
        }
        REVIEW_STATE_LEARNING | REVIEW_STATE_RELEARNING => {
            if rating == 4 || (rating == 3 && last_rating == Some(3)) {
                REVIEW_STATE_REVIEW
            } else {
                review_state_id
            }
        }
        REVIEW_STATE_REVIEW => {
            if rating == 1 {
                REVIEW_STATE_RELEARNING
            } else {
                REVIEW_STATE_REVIEW
            }
        }
        _ => review_state_id,
    }
}

fn learning_interval(rating: i16) -> Duration {
    match rating {
        1 => Duration::minutes(1),
        2 => Duration::minutes(4),
        3 => Duration::minutes(10),
        _ => unreachable!("rating 4 always transitions to review"),
    }
}
