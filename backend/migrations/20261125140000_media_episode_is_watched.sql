ALTER TABLE media_episodes
    ADD COLUMN is_watched boolean NOT NULL DEFAULT FALSE;

UPDATE media_episodes
SET is_watched = TRUE
WHERE watch_status_id = 3;

ALTER TABLE media_episodes
    DROP COLUMN watch_status_id,
    ADD CONSTRAINT media_episodes_watched_at_check CHECK (is_watched = (watched_at IS NOT NULL));
