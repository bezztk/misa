use chrono::{DateTime, Duration, Utc};
use fsrs::{FSRS, ItemState, MemoryState};

pub struct FlashcardReviewSchedule {
    pub stability: f32,
    pub difficulty: f32,
    pub reviewed_at: DateTime<Utc>,
    pub due_at: DateTime<Utc>,
    pub scheduled_days: i32,
}

pub fn calculate_review_schedule(
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
    let scheduled_days = next_state.interval.round().max(1.0) as i32;

    Ok(FlashcardReviewSchedule {
        stability: next_state.memory.stability,
        difficulty: next_state.memory.difficulty,
        reviewed_at,
        due_at: reviewed_at + Duration::days(i64::from(scheduled_days)),
        scheduled_days,
    })
}
