-- Add migration script here
ALTER TABLE calendar_entries
    ADD COLUMN title varchar(150) NOT NULL DEFAULT '',
ADD COLUMN note text;