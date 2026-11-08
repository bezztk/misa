-- Add migration script here
-- ==========================================
-- Lookup-Tabellen
-- ==========================================

CREATE TABLE topic_understanding_levels
(
    id           INTEGER PRIMARY KEY,
    name         VARCHAR(50) NOT NULL UNIQUE,
    display_name VARCHAR(50)
);

CREATE TABLE document_completeness_levels
(
    id           INTEGER PRIMARY KEY,
    name         VARCHAR(50) NOT NULL UNIQUE,
    display_name VARCHAR(50)
);

CREATE TABLE document_flashcard_statuses
(
    id           INTEGER PRIMARY KEY,
    name         VARCHAR(50) NOT NULL UNIQUE,
    display_name VARCHAR(50)
);

CREATE TABLE document_verification_statuses
(
    id           INTEGER PRIMARY KEY,
    name         VARCHAR(50) NOT NULL UNIQUE,
    display_name VARCHAR(50)
);

CREATE TABLE document_verification_levels
(
    id           INTEGER PRIMARY KEY,
    name         VARCHAR(50) NOT NULL UNIQUE,
    display_name VARCHAR(50)
);


-- ==========================================
-- Topics
-- ==========================================

CREATE TABLE topics
(
    entity_id              INTEGER PRIMARY KEY REFERENCES entities (id),
    name                   VARCHAR(150) NOT NULL,
    understanding_level_id INTEGER      NOT NULL DEFAULT 1
        REFERENCES topic_understanding_levels (id)
);


-- ==========================================
-- Documents
-- ==========================================

CREATE TABLE documents
(
    entity_id              INTEGER PRIMARY KEY REFERENCES entities (id),
    topic_id               INTEGER NOT NULL UNIQUE REFERENCES topics (entity_id),

    content                TEXT    NOT NULL DEFAULT '',

    completeness_level_id  INTEGER
        REFERENCES document_completeness_levels (id),

    flashcard_status_id    INTEGER NOT NULL DEFAULT 1
        REFERENCES document_flashcard_statuses (id),

    verification_status_id INTEGER NOT NULL DEFAULT 1
        REFERENCES document_verification_statuses (id),

    verification_level_id  INTEGER
        REFERENCES document_verification_levels (id),

    last_verified_at       TIMESTAMPTZ
);


-- ==========================================
-- Document Revisions
-- ==========================================

CREATE TABLE document_revisions
(
    id          INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    document_id INTEGER     NOT NULL REFERENCES documents (entity_id),

    content     TEXT        NOT NULL,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);


-- ==========================================
-- Seed-Daten
-- ==========================================

INSERT INTO topic_understanding_levels (id, name, display_name)
VALUES (1, 'basic', 'Grundlegend'),
       (2, 'developing', 'Im Aufbau'),
       (3, 'proficient', 'Sicher'),
       (4, 'advanced', 'Fortgeschritten'),
       (5, 'mastered', 'Gemeistert');

INSERT INTO document_completeness_levels (id, name, display_name)
VALUES (1, 'partial', 'Teilweise vollständig'),
       (2, 'substantial', 'Weitgehend vollständig'),
       (3, 'complete', 'Vollständig');

INSERT INTO document_flashcard_statuses (id, name, display_name)
VALUES (1, 'pending', 'Ausstehend'),
       (2, 'partially_completed', 'Teilweise abgeschlossen'),
       (3, 'completed', 'Abgeschlossen');

INSERT INTO document_verification_statuses (id, name, display_name)
VALUES (1, 'unverified', 'Ungeprüft'),
       (2, 'partially_verified', 'Teilweise geprüft'),
       (3, 'verified', 'Geprüft');

INSERT INTO document_verification_levels (id, name, display_name)
VALUES (1, 'self_reviewed', 'Eigenständig geprüft'),
       (2, 'cross_checked', 'Gegengeprüft'),
       (3, 'validated', 'Validiert');