use chrono::{DateTime, Duration, Utc};
use fsrs::{FSRS, ItemState, MemoryState};

use crate::models::flashcard::FlashcardReviewPreview;

pub struct FlashcardReviewSchedule {
    pub state: i16,
    pub learning_step: i16,
    pub stability: f32,
    pub difficulty: f32,
    pub reviewed_at: DateTime<Utc>,
    pub next_due_at: DateTime<Utc>,
    pub interval_days: f32,
}

const STATE_LEARNING: i16 = 1;
const STATE_REVIEW: i16 = 2;
const STATE_RELEARNING: i16 = 3;
const LEARNING_STEPS_MINUTES: [i64; 2] = [1, 10];
const MILLISECONDS_PER_DAY: f32 = 86_400_000.0;

pub fn calculate_review_schedule(
    state: i16,
    learning_step: i16,
    stability: Option<f32>,
    difficulty: Option<f32>,
    last_reviewed_at: Option<DateTime<Utc>>,
    desired_retention: f32,
    rating: i16,
    reviewed_at: DateTime<Utc>,
) -> fsrs::Result<FlashcardReviewSchedule> {
    let previous_state = stability
        .zip(difficulty)
        .map(|(stability, difficulty)| MemoryState {
            stability,
            difficulty,
        });
    let elapsed_days = last_reviewed_at
        .map(|last_reviewed_at| (reviewed_at - last_reviewed_at).num_days().max(0) as u32)
        .unwrap_or(0);
    let next_states =
        FSRS::default().next_states(previous_state, desired_retention, elapsed_days)?;
    let next_state: ItemState = match rating {
        1 => next_states.again,
        2 => next_states.hard,
        3 => next_states.good,
        4 => next_states.easy,
        _ => unreachable!("rating is validated by the handler"),
    };
    let fsrs_duration = Duration::milliseconds(
        (next_state.interval * MILLISECONDS_PER_DAY)
            .round()
            .max(1.0) as i64,
    );
    let current_step =
        usize::from(learning_step.clamp(0, (LEARNING_STEPS_MINUTES.len() - 1) as i16) as u16);
    let learning_state = if state == STATE_RELEARNING {
        STATE_RELEARNING
    } else {
        STATE_LEARNING
    };
    let step_duration = |step: usize| Duration::minutes(LEARNING_STEPS_MINUTES[step]);

    let (state, learning_step, interval) = if state == STATE_REVIEW {
        if rating == 1 {
            (STATE_RELEARNING, 0, step_duration(0))
        } else {
            (STATE_REVIEW, 0, fsrs_duration)
        }
    } else {
        match rating {
            1 => (learning_state, 0, step_duration(0)),
            2 => (learning_state, current_step as i16, Duration::minutes(4)),
            3 if current_step + 1 < LEARNING_STEPS_MINUTES.len() => (
                learning_state,
                (current_step + 1) as i16,
                step_duration(current_step + 1),
            ),
            3 | 4 => (STATE_REVIEW, 0, fsrs_duration),
            _ => unreachable!("rating is validated by the handler"),
        }
    };
    let interval_days = interval.num_milliseconds() as f32 / MILLISECONDS_PER_DAY;

    Ok(FlashcardReviewSchedule {
        state,
        learning_step,
        stability: next_state.memory.stability,
        difficulty: next_state.memory.difficulty,
        reviewed_at,
        next_due_at: reviewed_at + interval,
        interval_days,
    })
}

pub fn calculate_review_preview(
    state: i16,
    learning_step: i16,
    stability: Option<f32>,
    difficulty: Option<f32>,
    last_reviewed_at: Option<DateTime<Utc>>,
    desired_retention: f32,
    reviewed_at: DateTime<Utc>,
) -> fsrs::Result<FlashcardReviewPreview> {
    Ok(FlashcardReviewPreview {
        again: calculate_review_schedule(
            state,
            learning_step,
            stability,
            difficulty,
            last_reviewed_at,
            desired_retention,
            1,
            reviewed_at,
        )?
        .interval_days,
        hard: calculate_review_schedule(
            state,
            learning_step,
            stability,
            difficulty,
            last_reviewed_at,
            desired_retention,
            2,
            reviewed_at,
        )?
        .interval_days,
        good: calculate_review_schedule(
            state,
            learning_step,
            stability,
            difficulty,
            last_reviewed_at,
            desired_retention,
            3,
            reviewed_at,
        )?
        .interval_days,
        easy: calculate_review_schedule(
            state,
            learning_step,
            stability,
            difficulty,
            last_reviewed_at,
            desired_retention,
            4,
            reviewed_at,
        )?
        .interval_days,
    })
}
