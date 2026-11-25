CREATE TABLE source_categories
(
    id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name text NOT NULL UNIQUE
);

ALTER TABLE sources
    ADD COLUMN category_id integer REFERENCES source_categories (id) ON DELETE SET NULL,
    ADD COLUMN favicon bytea,
    ADD COLUMN favicon_mime_type text;
