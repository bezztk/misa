-- Add migration script here
ALTER TABLE calendar_entries
    RENAME COLUMN title TO name;