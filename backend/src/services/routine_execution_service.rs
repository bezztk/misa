use chrono::{DateTime, Datelike, Duration, NaiveDate, NaiveDateTime, NaiveTime, Utc, Weekday};
use sqlx::{FromRow, PgPool};
use std::collections::HashMap;

#[derive(FromRow)]
struct RoutineSource {
    id: i32,
    name: String,
    recurrence_rule_set: String,
    is_optional: bool,
    estimated_duration: Option<i32>,
}

enum Frequency {
    Daily,
    Weekly,
    Monthly,
    Yearly,
}

struct Rule {
    start_date: NaiveDate,
    time: Option<NaiveTime>,
    frequency: Frequency,
    interval: i32,
    by_day: Vec<Weekday>,
    by_month_day: Vec<u32>,
    by_month: Vec<u32>,
    count: Option<u32>,
    until_date: Option<NaiveDate>,
    until_utc: Option<DateTime<Utc>>,
}

pub async fn generate_next_executions(pool: &PgPool) -> Result<usize, sqlx::Error> {
    let routines = sqlx::query_as::<_, RoutineSource>(
        "SELECT r.id, st.name, r.recurrence_rule_set, r.is_optional,
                st.estimated_effort_minutes AS estimated_duration
         FROM systemtask_routines r
         JOIN systemtasks st ON st.id = r.systemtask_id
         WHERE r.is_active",
    )
    .fetch_all(pool)
    .await?;

    let today =
        sqlx::query_scalar::<_, NaiveDate>("SELECT (NOW() AT TIME ZONE 'Europe/Berlin')::date")
            .fetch_one(pool)
            .await?;
    let now = Utc::now();
    let mut generated = 0;

    for routine in routines {
        let Some(rule) = parse_rule(&routine.recurrence_rule_set) else {
            continue;
        };

        let has_future = if rule.time.is_some() {
            sqlx::query_scalar::<_, bool>(
                "SELECT EXISTS (
                     SELECT 1 FROM systemtask_routine_executions
                     WHERE routine_id = $1 AND scheduled_at > NOW()
                 )",
            )
            .bind(routine.id)
            .fetch_one(pool)
            .await?
        } else {
            sqlx::query_scalar::<_, bool>(
                "SELECT EXISTS (
                     SELECT 1 FROM systemtask_routine_executions
                     WHERE routine_id = $1 AND occurrence_on > $2
                           AND scheduled_at IS NULL
                 )",
            )
            .bind(routine.id)
            .bind(today)
            .fetch_one(pool)
            .await?
        };
        if has_future {
            continue;
        }

        if let Some((occurrence_on, scheduled_at)) =
            next_occurrence(pool, &rule, today, now).await?
            && create_execution(pool, &routine, occurrence_on, scheduled_at).await?
        {
            generated += 1;
        }
    }

    Ok(generated)
}

async fn next_occurrence(
    pool: &PgPool,
    rule: &Rule,
    today: NaiveDate,
    now: DateTime<Utc>,
) -> Result<Option<(NaiveDate, Option<DateTime<Utc>>)>, sqlx::Error> {
    let mut date = rule.start_date;
    let mut occurrence_number = 0;

    loop {
        let is_occurrence = date == rule.start_date || matches_rule(rule, date);
        if is_occurrence {
            occurrence_number += 1;
            if rule.count.is_some_and(|count| occurrence_number > count) {
                return Ok(None);
            }

            if rule.time.is_none() {
                if rule.until_date.is_some_and(|until| date > until) {
                    return Ok(None);
                }
                if date > today {
                    return Ok(Some((date, None)));
                }
            } else {
                let scheduled_at = berlin_date_time(pool, date, rule.time.unwrap()).await?;
                if rule.until_utc.is_some_and(|until| scheduled_at > until) {
                    return Ok(None);
                }
                if scheduled_at > now {
                    return Ok(Some((date, Some(scheduled_at))));
                }
            }
        }

        date += Duration::days(1);
    }
}

fn matches_rule(rule: &Rule, date: NaiveDate) -> bool {
    if date < rule.start_date
        || (!rule.by_month.is_empty() && !rule.by_month.contains(&date.month()))
        || (!rule.by_month_day.is_empty() && !rule.by_month_day.contains(&date.day()))
        || (!rule.by_day.is_empty() && !rule.by_day.contains(&date.weekday()))
    {
        return false;
    }

    match rule.frequency {
        Frequency::Daily => (date - rule.start_date).num_days() % i64::from(rule.interval) == 0,
        Frequency::Weekly => {
            let start_week = rule.start_date
                - Duration::days(i64::from(rule.start_date.weekday().num_days_from_monday()));
            let week = (date - start_week).num_days() / 7;
            week % i64::from(rule.interval) == 0
                && (!rule.by_day.is_empty() || date.weekday() == rule.start_date.weekday())
        }
        Frequency::Monthly => {
            let months = (date.year() - rule.start_date.year()) * 12 + date.month() as i32
                - rule.start_date.month() as i32;
            months % rule.interval == 0
                && (!rule.by_month_day.is_empty()
                    || !rule.by_day.is_empty()
                    || date.day() == rule.start_date.day())
        }
        Frequency::Yearly => {
            (date.year() - rule.start_date.year()) % rule.interval == 0
                && (!rule.by_month.is_empty() || date.month() == rule.start_date.month())
                && (!rule.by_month_day.is_empty()
                    || !rule.by_day.is_empty()
                    || date.day() == rule.start_date.day())
        }
    }
}

async fn berlin_date_time(
    pool: &PgPool,
    date: NaiveDate,
    time: NaiveTime,
) -> Result<DateTime<Utc>, sqlx::Error> {
    sqlx::query_scalar("SELECT (($1::date + $2::time) AT TIME ZONE 'Europe/Berlin')")
        .bind(date)
        .bind(time)
        .fetch_one(pool)
        .await
}

async fn create_execution(
    pool: &PgPool,
    routine: &RoutineSource,
    occurrence_on: NaiveDate,
    scheduled_at: Option<DateTime<Utc>>,
) -> Result<bool, sqlx::Error> {
    let mut transaction = pool.begin().await?;
    let execution_id = sqlx::query_scalar::<_, i32>(
        "INSERT INTO systemtask_routine_executions
             (routine_id, state_id, is_optional, occurrence_on, scheduled_at)
         VALUES ($1, 1, $2, $3, $4)
         ON CONFLICT (routine_id, occurrence_on) DO NOTHING
         RETURNING id",
    )
    .bind(routine.id)
    .bind(routine.is_optional)
    .bind(occurrence_on)
    .bind(scheduled_at)
    .fetch_optional(&mut *transaction)
    .await?;

    let Some(execution_id) = execution_id else {
        return Ok(false);
    };

    let start_at = match scheduled_at {
        Some(value) => value,
        None => berlin_date_time(pool, occurrence_on, NaiveTime::MIN).await?,
    };
    let end_at = routine
        .estimated_duration
        .map(|minutes| start_at + Duration::minutes(i64::from(minutes)));

    sqlx::query(
        "INSERT INTO calendar_entries
             (routine_execution_id, name, start_at, end_at, is_locked)
         VALUES ($1, $2, $3, $4, false)",
    )
    .bind(execution_id)
    .bind(&routine.name)
    .bind(start_at)
    .bind(end_at)
    .execute(&mut *transaction)
    .await?;

    transaction.commit().await?;
    Ok(true)
}

fn parse_rule(value: &str) -> Option<Rule> {
    let lines: Vec<&str> = value
        .lines()
        .map(|line| line.trim_end_matches('\r'))
        .collect();
    if lines.len() != 2 {
        return None;
    }

    let (start_date, time) = if let Some(value) = lines[0].strip_prefix("DTSTART;VALUE=DATE:") {
        (NaiveDate::parse_from_str(value, "%Y%m%d").ok()?, None)
    } else {
        let value = lines[0].strip_prefix("DTSTART;TZID=Europe/Berlin:")?;
        let date_time = NaiveDateTime::parse_from_str(value, "%Y%m%dT%H%M%S").ok()?;
        (date_time.date(), Some(date_time.time()))
    };

    let mut parts = HashMap::new();
    for part in lines[1].strip_prefix("RRULE:")?.split(';') {
        let (key, value) = part.split_once('=')?;
        parts.insert(key, value);
    }

    let frequency = match *parts.get("FREQ")? {
        "DAILY" => Frequency::Daily,
        "WEEKLY" => Frequency::Weekly,
        "MONTHLY" => Frequency::Monthly,
        "YEARLY" => Frequency::Yearly,
        _ => return None,
    };

    Some(Rule {
        start_date,
        time,
        frequency,
        interval: parts.get("INTERVAL").unwrap_or(&"1").parse().ok()?,
        by_day: parse_days(parts.get("BYDAY").copied())?,
        by_month_day: parse_numbers(parts.get("BYMONTHDAY").copied())?,
        by_month: parse_numbers(parts.get("BYMONTH").copied())?,
        count: parts.get("COUNT").and_then(|value| value.parse().ok()),
        until_date: parts
            .get("UNTIL")
            .filter(|value| !value.ends_with('Z'))
            .and_then(|value| NaiveDate::parse_from_str(value, "%Y%m%d").ok()),
        until_utc: parts
            .get("UNTIL")
            .filter(|value| value.ends_with('Z'))
            .and_then(|value| NaiveDateTime::parse_from_str(value, "%Y%m%dT%H%M%SZ").ok())
            .map(|value| value.and_utc()),
    })
}

fn parse_days(value: Option<&str>) -> Option<Vec<Weekday>> {
    value
        .map(|value| {
            value
                .split(',')
                .map(|day| match day {
                    "MO" => Some(Weekday::Mon),
                    "TU" => Some(Weekday::Tue),
                    "WE" => Some(Weekday::Wed),
                    "TH" => Some(Weekday::Thu),
                    "FR" => Some(Weekday::Fri),
                    "SA" => Some(Weekday::Sat),
                    "SU" => Some(Weekday::Sun),
                    _ => None,
                })
                .collect()
        })
        .unwrap_or_else(|| Some(Vec::new()))
}

fn parse_numbers(value: Option<&str>) -> Option<Vec<u32>> {
    value
        .map(|value| value.split(',').map(|number| number.parse().ok()).collect())
        .unwrap_or_else(|| Some(Vec::new()))
}
