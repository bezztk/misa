ALTER TABLE context_entities ADD UNIQUE (entity_id);
DROP INDEX idx_context_entities_entity_id;
