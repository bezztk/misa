-- Add migration script here
ALTER TABLE flashcards
    ADD COLUMN document_id INTEGER NOT NULL
        REFERENCES documents(entity_id);