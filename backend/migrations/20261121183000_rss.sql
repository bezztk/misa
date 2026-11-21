CREATE TABLE rss_feeds
(
    id              integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    url             text        NOT NULL UNIQUE,
    title           text        NOT NULL,
    last_fetched_at timestamptz,
    enabled         boolean     NOT NULL DEFAULT TRUE,
    is_hidden       boolean     NOT NULL DEFAULT FALSE,
    created_at      timestamptz NOT NULL DEFAULT NOW()
);

CREATE TABLE rss_entries
(
    id           integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    feed_id      integer     NOT NULL REFERENCES rss_feeds (id),
    external_id  text        NOT NULL,
    title        text        NOT NULL,
    url          text,
    summary      text,
    published_at timestamptz,
    is_read      boolean     NOT NULL DEFAULT FALSE,
    created_at   timestamptz NOT NULL DEFAULT NOW(),

    UNIQUE (feed_id, external_id)
);

CREATE INDEX idx_rss_entries_feed_published
    ON rss_entries (feed_id, published_at DESC);
