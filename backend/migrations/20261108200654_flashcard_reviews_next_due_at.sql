-- Add migration script here
ALTER TABLE flashcard_reviews
DROP COLUMN scheduled_days;

ALTER TABLE flashcard_reviews
    ADD COLUMN next_due_at TIMESTAMPTZ;