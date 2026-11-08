-- Add migration script here
CREATE TABLE flashcard_retention_levels
(
    id                INTEGER PRIMARY KEY,
    name              VARCHAR(50)   NOT NULL UNIQUE,
    display_name      VARCHAR(50),
    desired_retention NUMERIC(4, 3) NOT NULL
        CHECK (desired_retention > 0 AND desired_retention < 1)
);

CREATE TABLE flashcards
(
    id                  INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    front               TEXT     NOT NULL,
    back                TEXT     NOT NULL,
    hint                TEXT,
    explanation         TEXT,

    state               SMALLINT NOT NULL DEFAULT 0,
    retention_levels_id INTEGER  NOT NULL
        REFERENCES flashcard_retention_levels (id),

    stability           REAL,
    difficulty          REAL,
    last_reviewed_at    TIMESTAMPTZ,
    due_at              TIMESTAMPTZ,

    is_enabled BOOLEAN NOT NULL DEFAULT TRUE
);

CREATE TABLE flashcard_reviews
(
    id             INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    flashcard_id   INTEGER     NOT NULL
        REFERENCES flashcards (id) ON DELETE CASCADE,

    rating         SMALLINT    NOT NULL
        CHECK (rating BETWEEN 1 AND 4),
    reviewed_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    duration_ms    INTEGER CHECK (duration_ms >= 0),
    scheduled_days INTEGER CHECK (scheduled_days >= 0),
    stability      REAL,
    difficulty     REAL
);

INSERT INTO flashcard_retention_levels
    (id, name, display_name, desired_retention)
VALUES (1, 'supplementary', 'Zusatzwissen', 0.800),
       (2, 'general', 'Allgemeinwissen', 0.850),
       (3, 'specialized', 'Fachwissen', 0.900),
       (4, 'fundamental', 'Kernwissen', 0.930),
       (5, 'essential', 'Unverzichtbares Wissen', 0.950);