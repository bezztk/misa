-- Add migration script here
ALTER TABLE flashcards
    ADD COLUMN learning_step SMALLINT NOT NULL DEFAULT 0;