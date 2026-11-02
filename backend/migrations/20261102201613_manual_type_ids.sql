-- Add migration script here
ALTER TABLE types
    ALTER COLUMN id DROP IDENTITY IF EXISTS,
ALTER COLUMN id DROP DEFAULT;

INSERT INTO types (id, name)
VALUES (1, 'task');