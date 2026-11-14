-- Add migration script here
ALTER TABLE flashcard_reviews
    ALTER COLUMN duration_ms SET NOT NULL;

ALTER TABLE flashcard_reviews
DROP CONSTRAINT flashcard_reviews_flashcard_id_fkey,
    ADD CONSTRAINT flashcard_reviews_flashcard_id_fkey
        FOREIGN KEY (flashcard_id)
        REFERENCES flashcards (id)
        ON DELETE CASCADE;