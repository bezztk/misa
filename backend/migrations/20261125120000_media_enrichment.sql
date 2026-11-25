ALTER TABLE media
    ADD COLUMN trailer_url text,
    ADD COLUMN additional_backdrop_paths text[] NOT NULL DEFAULT '{}'
        CHECK (cardinality(additional_backdrop_paths) <= 4);

CREATE TABLE media_keywords
(
    id   integer PRIMARY KEY CHECK (id > 0),
    name text NOT NULL
);

CREATE TABLE media_keyword_assignments
(
    media_id   integer NOT NULL REFERENCES media (entity_id) ON DELETE CASCADE,
    keyword_id integer NOT NULL REFERENCES media_keywords (id),

    PRIMARY KEY (media_id, keyword_id)
);
CREATE INDEX idx_media_keyword_assignments_keyword_id ON media_keyword_assignments (keyword_id);
