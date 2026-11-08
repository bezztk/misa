-- Add migration script here
ALTER TABLE topics
    ADD COLUMN parent_topic_entity_id INT
        REFERENCES topics(entity_id);