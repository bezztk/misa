-- Add migration script here
CREATE TABLE item_hierarchy (
    child_id INTEGER PRIMARY KEY REFERENCES items(id) ON DELETE CASCADE,
    parent_id INTEGER NOT NULL REFERENCES items(id),
    CHECK (child_id <> parent_id)
);

ALTER TABLE items DROP COLUMN parent_id;