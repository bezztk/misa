-- Add migration script here
ALTER TABLE entities
    ADD COLUMN is_archived BOOLEAN NOT NULL DEFAULT FALSE,
ADD COLUMN is_deleted BOOLEAN NOT NULL DEFAULT FALSE;