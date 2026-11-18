use crate::repositories::routine_repository::{self, NewRoutineExecution};
use chrono::{
    DateTime, Datelike, Duration, NaiveDate, NaiveDateTime, NaiveTime, Timelike, Utc, Weekday,
};
use sqlx::PgPool;
use std::collections::HashMap;

const DAYS_IN_GREGORIAN_CYCLE: u32 = 146_097;
const WEEKS_IN_GREGORIAN_CYCLE: u32 = 20_871;
const MONTHS_IN_GREGORIAN_CYCLE: u32 = 4_800;
const YEARS_IN_GREGORIAN_CYCLE: u32 = 400;

#[derive(Clone, Copy)]
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
    interval: u32,
    by_day: Vec<Weekday>,
    by_month_day: Vec<u32>,
    by_month: Vec<u32>,
    count: Option<u32>,
    until_date: Option<NaiveDate>,
    until_utc: Option<DateTime<Utc>>,
}

struct Occurrence {
    date: NaiveDate,
    time: Option<NaiveTime>,
    until_utc: Option<DateTime<Utc>>,
}

pub async fn generate_next_executions(pool: &PgPool) -> Result<usize, sqlx::Error> {
    let routines = routine_repository::get_active_routine_execution_sources(pool).await?;
    let mut executions = Vec::new();

    for routine in routines {
        if routine.has_future_execution {
            continue;
        }

        let Some(rule) = parse_rule(&routine.recurrence_rule_set) else {
            continue;
        };
        let Some(occurrence) = next_occurrence(&rule, routine.local_now) else {
            continue;
        };

        executions.push(NewRoutineExecution {
            routine_id: routine.id,
            name: routine.name,
            is_optional: routine.is_optional,
            estimated_duration: routine.estimated_duration,
            occurrence_on: occurrence.date,
            time: occurrence.time,
            until_utc: occurrence.until_utc,
        });
    }

    routine_repository::create_routine_executions(pool, &executions).await
}

fn next_occurrence(rule: &Rule, local_now: NaiveDateTime) -> Option<Occurrence> {
    if let Some(count) = rule.count {
        let mut date = rule.start_date;

        for _ in 0..count {
            if !within_date_limit(rule, date) {
                return None;
            }
            if after_now(date, rule.time, local_now) {
                return Some(Occurrence {
                    date,
                    time: rule.time,
                    until_utc: rule.until_utc,
                });
            }

            date = next_matching_date(rule, date.succ_opt()?)?;
        }

        return None;
    }

    if within_date_limit(rule, rule.start_date) && after_now(rule.start_date, rule.time, local_now)
    {
        return Some(Occurrence {
            date: rule.start_date,
            time: rule.time,
            until_utc: rule.until_utc,
        });
    }

    let first_regular_date = rule.start_date.succ_opt()?;
    let first_search_date = if rule.time.is_some() {
        local_now.date().max(first_regular_date)
    } else {
        local_now.date().succ_opt()?.max(first_regular_date)
    };
    let mut date = next_matching_date(rule, first_search_date)?;

    while !after_now(date, rule.time, local_now) {
        date = next_matching_date(rule, date.succ_opt()?)?;
    }

    Some(Occurrence {
        date,
        time: rule.time,
        until_utc: rule.until_utc,
    })
}

fn after_now(date: NaiveDate, time: Option<NaiveTime>, local_now: NaiveDateTime) -> bool {
    match time {
        Some(time) => date.and_time(time) > local_now,
        None => date > local_now.date(),
    }
}

fn within_date_limit(rule: &Rule, date: NaiveDate) -> bool {
    search_limit(rule).is_none_or(|limit| date <= limit)
}

fn search_limit(rule: &Rule) -> Option<NaiveDate> {
    rule.until_date.or_else(|| {
        rule.until_utc
            .and_then(|until| until.date_naive().succ_opt())
    })
}

fn next_matching_date(rule: &Rule, from: NaiveDate) -> Option<NaiveDate> {
    if !within_date_limit(rule, from) {
        return None;
    }

    match rule.frequency {
        Frequency::Daily => next_daily_date(rule, from),
        Frequency::Weekly => next_weekly_date(rule, from),
        Frequency::Monthly => next_monthly_date(rule, from),
        Frequency::Yearly => next_yearly_date(rule, from),
    }
}

fn next_daily_date(rule: &Rule, from: NaiveDate) -> Option<NaiveDate> {
    let interval = i64::from(rule.interval);
    let elapsed = (from - rule.start_date).num_days().max(0);
    let occurrence = divide_rounding_up(elapsed, interval);
    let offset = occurrence.checked_mul(interval)?;
    let mut date = rule.start_date.checked_add_signed(Duration::days(offset))?;

    for _ in 0..cycle_steps(DAYS_IN_GREGORIAN_CYCLE, rule.interval) {
        if !within_date_limit(rule, date) {
            return None;
        }
        if date >= from && matches_filters(rule, date) {
            return Some(date);
        }
        date = date.checked_add_signed(Duration::days(interval))?;
    }

    None
}

fn next_weekly_date(rule: &Rule, from: NaiveDate) -> Option<NaiveDate> {
    let start_week = rule
        .start_date
        .checked_sub_signed(Duration::days(i64::from(
            rule.start_date.weekday().num_days_from_monday(),
        )))?;
    let elapsed_weeks = ((from - start_week).num_days().max(0)) / 7;
    let mut week = divide_rounding_up(elapsed_weeks, i64::from(rule.interval))
        .checked_mul(i64::from(rule.interval))?;
    let weekdays = if rule.by_day.is_empty() {
        vec![rule.start_date.weekday()]
    } else {
        rule.by_day.clone()
    };

    for _ in 0..cycle_steps(WEEKS_IN_GREGORIAN_CYCLE, rule.interval) {
        let week_start = start_week.checked_add_signed(Duration::weeks(week))?;
        if search_limit(rule).is_some_and(|limit| week_start > limit) {
            return None;
        }

        for weekday in &weekdays {
            let date = week_start
                .checked_add_signed(Duration::days(i64::from(weekday.num_days_from_monday())))?;
            if date >= from && date > rule.start_date && matches_filters(rule, date) {
                return Some(date);
            }
        }

        week = week.checked_add(i64::from(rule.interval))?;
    }

    None
}

fn next_monthly_date(rule: &Rule, from: NaiveDate) -> Option<NaiveDate> {
    let start_month = month_number(rule.start_date);
    let elapsed_months = (month_number(from) - start_month).max(0);
    let mut month = divide_rounding_up(elapsed_months, i64::from(rule.interval))
        .checked_mul(i64::from(rule.interval))?;

    for _ in 0..cycle_steps(MONTHS_IN_GREGORIAN_CYCLE, rule.interval) {
        let (year, month_number) = year_and_month(start_month.checked_add(month)?)?;
        let month_start = NaiveDate::from_ymd_opt(year, month_number, 1)?;
        if search_limit(rule).is_some_and(|limit| month_start > limit) {
            return None;
        }

        for day in candidate_days(rule) {
            let Some(date) = NaiveDate::from_ymd_opt(year, month_number, day) else {
                continue;
            };
            if date >= from && date > rule.start_date && matches_filters(rule, date) {
                return Some(date);
            }
        }

        month = month.checked_add(i64::from(rule.interval))?;
    }

    None
}

fn next_yearly_date(rule: &Rule, from: NaiveDate) -> Option<NaiveDate> {
    let elapsed_years = (from.year() - rule.start_date.year()).max(0);
    let mut year_offset = divide_rounding_up(i64::from(elapsed_years), i64::from(rule.interval))
        .checked_mul(i64::from(rule.interval))?;
    let months = if rule.by_month.is_empty() {
        vec![rule.start_date.month()]
    } else {
        rule.by_month.clone()
    };

    for _ in 0..cycle_steps(YEARS_IN_GREGORIAN_CYCLE, rule.interval) {
        let year = i64::from(rule.start_date.year()).checked_add(year_offset)?;
        let year = i32::try_from(year).ok()?;
        let year_start = NaiveDate::from_ymd_opt(year, 1, 1)?;
        if search_limit(rule).is_some_and(|limit| year_start > limit) {
            return None;
        }

        for month in &months {
            for day in candidate_days(rule) {
                let Some(date) = NaiveDate::from_ymd_opt(year, *month, day) else {
                    continue;
                };
                if date >= from && date > rule.start_date && matches_filters(rule, date) {
                    return Some(date);
                }
            }
        }

        year_offset = year_offset.checked_add(i64::from(rule.interval))?;
    }

    None
}

fn candidate_days(rule: &Rule) -> Vec<u32> {
    if !rule.by_month_day.is_empty() {
        rule.by_month_day.clone()
    } else if !rule.by_day.is_empty() {
        (1..=31).collect()
    } else {
        vec![rule.start_date.day()]
    }
}

fn matches_filters(rule: &Rule, date: NaiveDate) -> bool {
    (rule.by_month.is_empty() || rule.by_month.contains(&date.month()))
        && (rule.by_month_day.is_empty() || rule.by_month_day.contains(&date.day()))
        && (rule.by_day.is_empty() || rule.by_day.contains(&date.weekday()))
}

fn month_number(date: NaiveDate) -> i64 {
    i64::from(date.year()) * 12 + i64::from(date.month0())
}

fn year_and_month(month: i64) -> Option<(i32, u32)> {
    let year = i32::try_from(month.div_euclid(12)).ok()?;
    let month = u32::try_from(month.rem_euclid(12)).ok()? + 1;
    Some((year, month))
}

fn divide_rounding_up(value: i64, divisor: i64) -> i64 {
    if value == 0 {
        0
    } else {
        (value - 1) / divisor + 1
    }
}

fn cycle_steps(cycle: u32, interval: u32) -> u32 {
    cycle / greatest_common_divisor(cycle, interval % cycle)
}

fn greatest_common_divisor(mut left: u32, mut right: u32) -> u32 {
    while right != 0 {
        (left, right) = (right, left % right);
    }
    left
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
        if parts.insert(key, value).is_some() {
            return None;
        }
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
    if parts.keys().any(|key| !supported.contains(key))
        || (parts.contains_key("COUNT") && parts.contains_key("UNTIL"))
    {
        return None;
    }

    let frequency = match *parts.get("FREQ")? {
        "DAILY" => Frequency::Daily,
        "WEEKLY" => Frequency::Weekly,
        "MONTHLY" => Frequency::Monthly,
        "YEARLY" => Frequency::Yearly,
        _ => return None,
    };
    let interval = parts
        .get("INTERVAL")
        .map_or(Some(1), |value| value.parse().ok())?;
    if interval == 0 {
        return None;
    }

    let count = match parts.get("COUNT") {
        Some(value) => Some(value.parse::<u32>().ok()?),
        None => None,
    };
    if count == Some(0) {
        return None;
    }

    let by_day = parse_days(parts.get("BYDAY").copied())?;
    let by_month_day = parse_numbers(parts.get("BYMONTHDAY").copied(), 1, 31)?;
    let by_month = parse_numbers(parts.get("BYMONTH").copied(), 1, 12)?;

    let (until_date, until_utc) = match time {
        None => {
            if parts.contains_key("BYHOUR") || parts.contains_key("BYMINUTE") {
                return None;
            }
            let until = match parts.get("UNTIL") {
                Some(value) => Some(NaiveDate::parse_from_str(value, "%Y%m%d").ok()?),
                None => None,
            };
            (until, None)
        }
        Some(time) => {
            if parts
                .get("BYHOUR")
                .and_then(|value| value.parse::<u32>().ok())
                != Some(time.hour())
                || parts
                    .get("BYMINUTE")
                    .and_then(|value| value.parse::<u32>().ok())
                    != Some(time.minute())
            {
                return None;
            }
            let until = match parts.get("UNTIL") {
                Some(value) => Some(
                    NaiveDateTime::parse_from_str(value, "%Y%m%dT%H%M%SZ")
                        .ok()?
                        .and_utc(),
                ),
                None => None,
            };
            (None, until)
        }
    };

    Some(Rule {
        start_date,
        time,
        frequency,
        interval,
        by_day,
        by_month_day,
        by_month,
        count,
        until_date,
        until_utc,
    })
}

fn parse_days(value: Option<&str>) -> Option<Vec<Weekday>> {
    let mut days = value
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
        .unwrap_or_else(|| Some(Vec::new()))?;
    days.sort_by_key(|day| day.num_days_from_monday());
    days.dedup();
    Some(days)
}

fn parse_numbers(value: Option<&str>, minimum: u32, maximum: u32) -> Option<Vec<u32>> {
    let mut numbers = value
        .map(|value| {
            value
                .split(',')
                .map(|number| {
                    number
                        .parse::<u32>()
                        .ok()
                        .filter(|number| *number >= minimum && *number <= maximum)
                })
                .collect()
        })
        .unwrap_or_else(|| Some(Vec::new()))?;
    numbers.sort_unstable();
    numbers.dedup();
    Some(numbers)
}
