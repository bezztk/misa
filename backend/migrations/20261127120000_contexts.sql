CREATE TABLE contexts
(
    id    integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    title varchar(150) NOT NULL
);

CREATE TABLE context_entities
(
    context_id integer NOT NULL REFERENCES contexts (id) ON DELETE CASCADE,
    entity_id  integer NOT NULL REFERENCES entities (id) ON DELETE CASCADE,

    PRIMARY KEY (context_id, entity_id)
);
CREATE INDEX idx_context_entities_entity_id ON context_entities (entity_id);

ALTER TABLE systemtasks DROP COLUMN group_id;
DROP TABLE systemtask_groups;
