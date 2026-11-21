CREATE TABLE rss_categories
(
    id    integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name  varchar(100) NOT NULL UNIQUE,
    icon  bytea,
    color varchar(7)   NOT NULL
);

ALTER TABLE rss_feeds
    ADD COLUMN category_id integer REFERENCES rss_categories (id) ON DELETE SET NULL;
