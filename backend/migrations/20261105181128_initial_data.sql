-- Entity-Typen
INSERT INTO types (id, name)
VALUES (1, 'task'),
       (2, 'calendar_entry');

-- Aufgabengruppen
INSERT INTO task_groups (name)
VALUES ('Allgemein'),
       ('Persönliches'),
       ('Einkauf'),
       ('Schule');

-- Aufgabenstatus
INSERT INTO task_states (id, name)
VALUES (1, 'open'),
       (2, 'in_progress'),
       (3, 'completed'),
       (4, 'cancelled');

-- Aufgabenprioritäten
INSERT INTO task_priorities (id, name)
VALUES (1, 'low'),
       (2, 'normal'),
       (3, 'high'),
       (4, 'critical');

-- Tasks
ALTER TABLE tasks
DROP CONSTRAINT tasks_entity_id_fkey;

ALTER TABLE tasks
    ADD CONSTRAINT tasks_entity_id_fkey
        FOREIGN KEY (entity_id)
            REFERENCES entities(id) ON DELETE CASCADE;

-- Calendar Entries
ALTER TABLE calendar_entries
DROP CONSTRAINT calendar_entries_entity_id_fkey;

ALTER TABLE calendar_entries
    ADD CONSTRAINT calendar_entries_entity_id_fkey
        FOREIGN KEY (entity_id)
            REFERENCES entities(id) ON DELETE CASCADE;

-- Source Entity
ALTER TABLE calendar_entries
DROP CONSTRAINT calendar_entries_source_entity_id_fkey;

ALTER TABLE calendar_entries
    ADD CONSTRAINT calendar_entries_source_entity_id_fkey
        FOREIGN KEY (source_entity_id)
            REFERENCES entities(id) ON DELETE CASCADE;