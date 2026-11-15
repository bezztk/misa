use crate::api::error::{ApiError, ApiResult};
use crate::models::routine::{DomainNodeRoutine, SaveDomainNodeRoutineRequest};
use crate::repositories::routine_repository::{self, CreateRoutineResult};
use crate::validation::valid_name;
use axum::Json;
use axum::extract::{Path, State};
use axum::http::StatusCode;
use chrono::{NaiveDate, NaiveDateTime, Timelike};
use sqlx::PgPool;
use std::collections::HashMap;

pub async fn get_domain_node_routine(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
) -> ApiResult<Json<DomainNodeRoutine>> {
    routine_repository::get_domain_node_routine(&pool, node_id)
        .await?
        .map(Json)
        .ok_or(ApiError::NotFound)
}

pub async fn create_domain_node_routine(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<SaveDomainNodeRoutineRequest>,
) -> ApiResult<(StatusCode, Json<DomainNodeRoutine>)> {
    validate(&payload)?;

    match routine_repository::create_domain_node_routine(&pool, node_id, &payload).await? {
        CreateRoutineResult::Created(routine) => Ok((StatusCode::CREATED, Json(routine))),
        CreateRoutineResult::NodeNotFound => Err(ApiError::NotFound),
        CreateRoutineResult::RoutineExists => Err(ApiError::Conflict),
    }
}

pub async fn update_domain_node_routine(
    State(pool): State<PgPool>,
    Path(node_id): Path<i32>,
    Json(payload): Json<SaveDomainNodeRoutineRequest>,
) -> ApiResult<Json<DomainNodeRoutine>> {
    validate(&payload)?;

    routine_repository::update_domain_node_routine(&pool, node_id, &payload)
        .await?
        .map(Json)
        .ok_or(ApiError::NotFound)
}

fn validate(payload: &SaveDomainNodeRoutineRequest) -> ApiResult<()> {
    if !valid_name(&payload.name, 150)
        || payload.timezone != "Europe/Berlin"
        || payload.estimated_duration <= 0
        || !valid_rrule_set(&payload.recurrence_rule_set)
    {
        return Err(ApiError::BadRequest);
    }
    Ok(())
}

fn valid_rrule_set(value: &str) -> bool {
    let lines: Vec<&str> = value
        .lines()
        .map(|line| line.trim_end_matches('\r'))
        .collect();
    if lines.len() != 2 || !lines[1].starts_with("RRULE:") {
        return false;
    }

    let start_time = if let Some(date) = lines[0].strip_prefix("DTSTART;VALUE=DATE:") {
        if NaiveDate::parse_from_str(date, "%Y%m%d").is_err() {
            return false;
        }
        None
    } else if let Some(date_time) = lines[0].strip_prefix("DTSTART;TZID=Europe/Berlin:") {
        let Ok(date_time) = NaiveDateTime::parse_from_str(date_time, "%Y%m%dT%H%M%S") else {
            return false;
        };
        Some((date_time.hour(), date_time.minute()))
    } else {
        return false;
    };

    let mut parts = HashMap::new();
    for part in lines[1].trim_start_matches("RRULE:").split(';') {
        let Some((key, value)) = part.split_once('=') else {
            return false;
        };
        if parts.insert(key, value).is_some() {
            return false;
        }
    }

    if !matches!(
        parts.get("FREQ"),
        Some(&"DAILY" | &"WEEKLY" | &"MONTHLY" | &"YEARLY")
    ) || parts
        .get("INTERVAL")
        .is_some_and(|value| value.parse::<u32>().map_or(true, |number| number == 0))
        || !valid_list(
            parts.get("BYDAY"),
            &["MO", "TU", "WE", "TH", "FR", "SA", "SU"],
        )
        || !valid_number_list(parts.get("BYMONTHDAY"), 1, 31)
        || !valid_number_list(parts.get("BYMONTH"), 1, 12)
        || parts
            .get("COUNT")
            .is_some_and(|value| value.parse::<u32>().map_or(true, |number| number == 0))
        || (parts.contains_key("COUNT") && parts.contains_key("UNTIL"))
    {
        return false;
    }

    let supported = [
        "FREQ",
        "INTERVAL",
        "BYDAY",
        "BYMONTHDAY",
        "BYMONTH",
        "COUNT",
        "UNTIL",
        "BYHOUR",
        "BYMINUTE",
    ];
    if parts.keys().any(|key| !supported.contains(key)) {
        return false;
    }

    match start_time {
        None => {
            if parts.contains_key("BYHOUR") || parts.contains_key("BYMINUTE") {
                return false;
            }
            parts
                .get("UNTIL")
                .is_none_or(|value| NaiveDate::parse_from_str(value, "%Y%m%d").is_ok())
        }
        Some((hour, minute)) => {
            if parts
                .get("BYHOUR")
                .and_then(|value| value.parse::<u32>().ok())
                != Some(hour)
                || parts
                    .get("BYMINUTE")
                    .and_then(|value| value.parse::<u32>().ok())
                    != Some(minute)
            {
                return false;
            }
            parts
                .get("UNTIL")
                .is_none_or(|value| NaiveDateTime::parse_from_str(value, "%Y%m%dT%H%M%SZ").is_ok())
        }
    }
}

fn valid_list(value: Option<&&str>, allowed: &[&str]) -> bool {
    value.is_none_or(|value| {
        !value.is_empty() && value.split(',').all(|item| allowed.contains(&item))
    })
}

fn valid_number_list(value: Option<&&str>, minimum: u32, maximum: u32) -> bool {
    value.is_none_or(|value| {
        !value.is_empty()
            && value.split(',').all(|item| {
                item.parse::<u32>()
                    .is_ok_and(|number| number >= minimum && number <= maximum)
            })
    })
}
