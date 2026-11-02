-- Add migration script here
-- Task Groups
CREATE TABLE task_groups (
    id INTEGER PRIMARY KEY,
    name VARCHAR(50) NOT NULL UNIQUE
);

INSERT INTO task_groups (id, name) VALUES
    (1, 'Allgemein'),
    (2, 'Persönliches'),
    (3, 'Einkauf'),
    (4, 'Schule');

-- Task States
CREATE TABLE task_states (
    id INTEGER PRIMARY KEY,
    name VARCHAR(50) NOT NULL UNIQUE
);

INSERT INTO task_states (id, name) VALUES
    (1, 'open'),
    (2, 'in_progress'),
    (3, 'completed'),
    (4, 'cancelled');

-- Tasks
CREATE TABLE tasks (
    item_id BIGINT PRIMARY KEY REFERENCES items(id) ON DELETE CASCADE,
    group_id INTEGER REFERENCES task_groups(id) ON DELETE SET NULL,
    state_id INTEGER NOT NULL DEFAULT 1 REFERENCES task_states(id),
    name VARCHAR(150) NOT NULL,
    description TEXT,
    completed_at TIMESTAMPTZ
);

DROP TRIGGER IF EXISTS trg_update_item_metadata ON items;
DROP FUNCTION IF EXISTS update_item_metadata();