use chrono::{DateTime, Duration, Utc};
use fsrs::{FSRS, ItemState, MemoryState};

use crate::models::flashcard::{FlashcardReviewPreview, FlashcardReviewStateId};

pub struct FlashcardReviewSchedule {
    pub review_state_id: i32,
    pub stability: f32,
    pub difficulty: f32,
    pub reviewed_at: DateTime<Utc>,
    pub next_due_at: DateTime<Utc>,
    pub interval_days: f32,
}

const REVIEW_STATE_NEW: i32 = FlashcardReviewStateId::New as i32;
const REVIEW_STATE_LEARNING: i32 = FlashcardReviewStateId::Learning as i32;
const REVIEW_STATE_REVIEW: i32 = FlashcardReviewStateId::Review as i32;
const REVIEW_STATE_RELEARNING: i32 = FlashcardReviewStateId::Relearning as i32;
const MILLISECONDS_PER_DAY: f32 = 86_400_000.0;

pub fn calculate_review_schedule(
    review_state_id: i32,
    last_rating: Option<i16>,
    stability: Option<f32>,
    difficulty: Option<f32>,
    last_reviewed_at: Option<DateTime<Utc>>,
    desired_retention: f32,
    rating: i16,
    reviewed_at: DateTime<Utc>,
) -> fsrs::Result<FlashcardReviewSchedule> {
    let previous_memory = stability
        .zip(difficulty)
        .map(|(stability, difficulty)| MemoryState {
            stability,
            difficulty,
        });
    let elapsed_days = last_reviewed_at
        .map(|last_reviewed_at| (reviewed_at - last_reviewed_at).num_days().max(0) as u32)
        .unwrap_or(0);
    let next_states =
        FSRS::default().next_states(previous_memory, desired_retention, elapsed_days)?;
    let next_memory: ItemState = match rating {
        1 => next_states.again,
        2 => next_states.hard,
        3 => next_states.good,
        4 => next_states.easy,
        _ => unreachable!("rating is validated by the handler"),
    };

    let next_review_state_id = next_review_state(review_state_id, last_rating, rating);
    let fsrs_duration = Duration::milliseconds(
        (next_memory.interval * MILLISECONDS_PER_DAY)
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
        reviewed_at,
        next_due_at: reviewed_at + interval,
        interval_days,
    })
}

pub fn calculate_review_preview(
    review_state_id: i32,
    last_rating: Option<i16>,
    stability: Option<f32>,
    difficulty: Option<f32>,
    last_reviewed_at: Option<DateTime<Utc>>,
    desired_retention: f32,
    reviewed_at: DateTime<Utc>,
) -> fsrs::Result<FlashcardReviewPreview> {
    let interval_for = |rating| {
        calculate_review_schedule(
            review_state_id,
            last_rating,
            stability,
            difficulty,
            last_reviewed_at,
            desired_retention,
            rating,
            reviewed_at,
        )
        .map(|schedule| schedule.interval_days)
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
