-- Add migration script here
ALTER TABLE calendar_entries
DROP COLUMN actual_start_at,
    DROP COLUMN actual_end_at;

ALTER TABLE calendar_entries
    RENAME COLUMN planned_start_at TO start_at;

ALTER TABLE calendar_entries
    RENAME COLUMN planned_end_at TO end_at;