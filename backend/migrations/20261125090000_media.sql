INSERT INTO entity_types (id, name)
VALUES (10, 'media');

CREATE TABLE media_types
(
    id   integer PRIMARY KEY,
    name varchar(50) NOT NULL UNIQUE
);
INSERT INTO media_types (id, name)
VALUES (1, 'movie'),
       (2, 'tv');

CREATE TABLE media_watch_statuses
(
    id   integer PRIMARY KEY,
    name varchar(50) NOT NULL UNIQUE
);
INSERT INTO media_watch_statuses (id, name)
VALUES (1, 'planned'),
       (2, 'watching'),
       (3, 'completed'),
       (4, 'paused'),
       (5, 'dropped');

CREATE TABLE media
(
    entity_id         integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    tmdb_id           integer NOT NULL CHECK (tmdb_id > 0),
    media_type_id     integer NOT NULL REFERENCES media_types (id),
    title             text,
    original_title    text,
    description       text,
    poster_path       text,
    backdrop_path     text,
    release_date      date,
    runtime_minutes   integer CHECK (runtime_minutes > 0),
    production_status text,
    personal_rating   integer CHECK (personal_rating BETWEEN 1 AND 10),
    personal_notes    text,
    watch_status_id   integer NOT NULL DEFAULT 1 REFERENCES media_watch_statuses (id),
    watch_count       integer NOT NULL DEFAULT 0 CHECK (watch_count >= 0),
    completed_at      timestamptz,
    last_synced_at    timestamptz NOT NULL DEFAULT NOW(),

    UNIQUE (tmdb_id, media_type_id),
    CHECK (runtime_minutes IS NULL OR media_type_id = 1),
    CHECK ((watch_status_id = 3) = (completed_at IS NOT NULL))
);

CREATE TABLE media_seasons
(
    id            integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    media_id      integer NOT NULL REFERENCES media (entity_id) ON DELETE CASCADE,
    tmdb_id       integer NOT NULL UNIQUE CHECK (tmdb_id > 0),
    season_number integer NOT NULL CHECK (season_number >= 0),
    title         text,
    description   text,
    poster_path   text,
    release_date  date
);
CREATE INDEX idx_media_seasons_media_number ON media_seasons (media_id, season_number);

CREATE TABLE media_episodes
(
    id              integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    season_id       integer NOT NULL REFERENCES media_seasons (id) ON DELETE CASCADE,
    tmdb_id         integer NOT NULL UNIQUE CHECK (tmdb_id > 0),
    episode_number  integer NOT NULL CHECK (episode_number >= 0),
    title           text,
    description     text,
    runtime_minutes integer CHECK (runtime_minutes > 0),
    release_date    date,
    watch_status_id integer NOT NULL DEFAULT 1 REFERENCES media_watch_statuses (id),
    watched_at      timestamptz,

    CHECK ((watch_status_id = 3) = (watched_at IS NOT NULL))
);
CREATE INDEX idx_media_episodes_season_number ON media_episodes (season_id, episode_number);

CREATE TABLE media_genres
(
    id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name text NOT NULL UNIQUE
);

CREATE TABLE media_genre_assignments
(
    media_id integer NOT NULL REFERENCES media (entity_id) ON DELETE CASCADE,
    genre_id integer NOT NULL REFERENCES media_genres (id),

    PRIMARY KEY (media_id, genre_id)
);
CREATE INDEX idx_media_genre_assignments_genre_id ON media_genre_assignments (genre_id);
