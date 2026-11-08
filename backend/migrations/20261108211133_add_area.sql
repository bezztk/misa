-- Add migration script here
CREATE TABLE development_areas
(
    id                INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name              VARCHAR(50) NOT NULL UNIQUE,

    background_color  VARCHAR(7),
    banner_image_path TEXT,
    icon              VARCHAR(50),

    sort_order        INTEGER     NOT NULL DEFAULT 0
);

ALTER TABLE topics
    ADD COLUMN development_area_id INTEGER
        REFERENCES development_areas (id);