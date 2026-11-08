-- Add migration script here
-- Task States
ALTER TABLE task_states
    ADD COLUMN display_name VARCHAR(50);

DELETE
FROM task_states;

INSERT INTO task_states (id, name, display_name)
VALUES (1, 'open', 'Offen'),
       (2, 'in_progress', 'In Bearbeitung'),
       (3, 'completed', 'Abgeschlossen'),
       (4, 'cancelled', 'Abgebrochen');


-- Task Priorities
ALTER TABLE task_priorities
    ADD COLUMN display_name VARCHAR(50);

DELETE
FROM task_priorities;

INSERT INTO task_priorities (id, name, display_name)
VALUES (1, 'low', 'Niedrig'),
       (2, 'normal', 'Normal'),
       (3, 'high', 'Hoch'),
       (4, 'critical', 'Kritisch');


-- Tasks
ALTER TABLE tasks
    ADD COLUMN source_entity_id INT
        REFERENCES entities (id);


-- Calendar Entries
ALTER TABLE calendar_entries
    ADD COLUMN is_locked BOOLEAN NOT NULL DEFAULT FALSE,
ADD COLUMN estimated_duration INT;