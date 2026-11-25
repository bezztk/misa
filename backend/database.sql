--region Zentrale Entitaeten
CREATE TABLE entity_types
(
    id   integer PRIMARY KEY,
    name varchar(50) NOT NULL UNIQUE
);
INSERT INTO entity_types (id, name)
VALUES (1, 'domain_node'),
       (2, 'systemtask'),
       (3, 'progression'),
       (4, 'pursuit'),
       (6, 'flashcard'),
       (7, 'calendar_entry'),
       (8, 'attachment'),
       (9, 'ideas'),
       (10, 'media');

CREATE TABLE entities
(
    id         integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    type_id    integer     NOT NULL REFERENCES entity_types (id),
    version    integer     NOT NULL DEFAULT 1,
    created_at timestamptz NOT NULL DEFAULT NOW(),
    updated_at timestamptz NOT NULL DEFAULT NOW()
);

CREATE TABLE entity_fields
(
    id             integer PRIMARY KEY,
    entity_type_id integer     NOT NULL REFERENCES entity_types (id),
    field_type_id  integer     NOT NULL,
    name           varchar(50) NOT NULL,

    UNIQUE (entity_type_id, field_type_id),
    UNIQUE (entity_type_id, name)
);
INSERT INTO entity_fields (id, entity_type_id, field_type_id, name)
VALUES (1, 1, 1, 'name'),
       (2, 1, 2, 'knowledge_level_id'),
       (3, 1, 3, 'completeness_level_id'),
       (4, 1, 4, 'flashcard_status_id'),
       (5, 1, 5, 'verification_status_id'),
       (6, 1, 6, 'verification_level_id'),
       (7, 2, 1, 'name'),
       (8, 2, 2, 'state_id'),
       (9, 2, 3, 'priority_id'),
       (12, 6, 1, 'front'),
       (13, 6, 2, 'back'),
       (14, 3, 1, 'name'),
       (15, 3, 2, 'personal_relevance_id'),
       (16, 3, 3, 'lasting_impact_id'),
       (17, 9, 1, 'title'),
       (18, 9, 2, 'content'),
       (19, 9, 3, 'phase_id'),
       (20, 9, 4, 'state_id');

CREATE TABLE entity_field_versions
(
    entity_id  integer     NOT NULL REFERENCES entities (id) ON DELETE CASCADE,
    version    integer     NOT NULL,
    field_id   integer     NOT NULL REFERENCES entity_fields (id),
    old_value  jsonb       NOT NULL,
    new_value  jsonb       NOT NULL,
    changed_at timestamptz NOT NULL DEFAULT NOW(),

    PRIMARY KEY (entity_id, version)
);

CREATE TABLE comments
(
    id         integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    entity_id  integer     NOT NULL REFERENCES entities (id) ON DELETE CASCADE,
    content    text        NOT NULL,
    created_at timestamptz NOT NULL DEFAULT NOW(),
    updated_at timestamptz NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_comments_entity_id ON comments (entity_id);
--endregion

--region Ideen
CREATE TABLE idea_phases
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE,
    description  text        NOT NULL
);
INSERT INTO idea_phases (id, name, display_name, description)
VALUES (1, 'preparation', 'Vorbereitung', 'Die Idee wird erstmals erfasst und bewusst betrachtet. Erste Gedanken, Informationen und mögliche Ansätze werden gesammelt.'),
       (2, 'incubation', 'Inkubation', 'Die Idee ruht, ohne dass eine aktive Beschäftigung erforderlich ist. Weitere Gedanken und Zusammenhänge können sich mit zeitlichem Abstand entwickeln.'),
       (3, 'intimation', 'Vorahnung', 'Es entsteht das Gefühl, dass sich eine neue Erkenntnis oder Lösung anbahnt, ohne dass diese bereits vollständig formuliert werden kann.'),
       (4, 'illumination', 'Erkenntnis', 'Eine neue Erkenntnis, ein Zusammenhang oder ein Lösungsansatz wird bewusst erkannt und kann festgehalten werden.'),
       (5, 'verification', 'Überprüfung', 'Die Idee wird kritisch überprüft und hinsichtlich ihrer Umsetzbarkeit, ihres Nutzens und ihrer möglichen Konsequenzen bewertet.');

CREATE TABLE idea_states
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE,
    description  text        NOT NULL
);
INSERT INTO idea_states (id, name, display_name, description)
VALUES (1, 'active', 'Aktiv', 'Die Idee ist weiterhin relevant und kann jederzeit weiterentwickelt werden.'),
       (2, 'done', 'Abgeschlossen', 'Die Idee wurde ausreichend ausgearbeitet, umgesetzt oder anderweitig abgeschlossen.'),
       (3, 'canceled', 'Verworfen', 'Die Idee wird nicht weiterverfolgt, bleibt aber zur Dokumentation erhalten.');

CREATE TABLE ideas
(
    id           integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    title        varchar(150) NOT NULL,
    content      text         NOT NULL,
    phase_id     integer      NOT NULL DEFAULT 1 REFERENCES idea_phases (id),
    state_id     integer      NOT NULL DEFAULT 1 REFERENCES idea_states (id),
    completed_at timestamptz
);
--endregion

--region Globale Metadaten
CREATE TABLE personal_relevance_levels
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO personal_relevance_levels (id, name, display_name)
VALUES (1, 'insignificant', 'Unbedeutend'),
       (2, 'secondary', 'Nachrangig'),
       (3, 'significant', 'Bedeutsam'),
       (4, 'essential', 'Wesentlich'),
       (5, 'identity_defining', 'Identitätsprägend');

CREATE TABLE lasting_impacts
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO lasting_impacts (id, name, display_name)
VALUES (1, 'negligible', 'Vernachlässigbar'),
       (2, 'minor', 'Geringfügig'),
       (3, 'noticeable', 'Spürbar'),
       (4, 'profound', 'Tiefgreifend'),
       (5, 'transformative', 'Lebensverändernd');
--endregion

--region Verwirklichung

--region Knowledge Level
CREATE TABLE knowledge_levels
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE,
    description  text        NOT NULL
);
INSERT INTO knowledge_levels (id, name, display_name, description)
VALUES (0, 'none', 'Keine Kenntnisse', 'Kein relevantes Wissen vorhanden.'),
       (1, 'elementary', 'Elementar', 'Grundlegendes Allgemeinwissen.'),
       (2, 'basic', 'Grundlegend', 'Grundlegendes fachbezogenes Wissen.'),
       (3, 'sound', 'Fundiert', 'Kenntnisse von Fakten, Prinzipien, Verfahren und allgemeinen Begriffen.'),
       (4, 'advanced', 'Fortgeschritten', 'Breites theoretisches und praktisches Wissen.'),
       (5, 'comprehensive', 'Umfassend', 'Umfassendes, spezialisiertes Fachwissen und Bewusstsein für dessen Grenzen.'),
       (6, 'in_depth', 'Vertieft', 'Fortgeschrittenes Wissen und kritisches Verständnis von Theorien und Prinzipien.'),
       (7, 'highly_specialized', 'Hoch spezialisiert', 'Hoch spezialisiertes Wissen und kritisches Bewusstsein für Wissensgrenzen und fachübergreifende Zusammenhänge.'),
       (8, 'research_frontier', 'Wissenschaftlich führend', 'Wissen an den äußersten Grenzen eines Fachgebietes und an Schnittstellen zwischen Fachgebieten.');
--endregion

--region Metadaten des Dokumentes
CREATE TABLE document_completeness_levels
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO document_completeness_levels (id, name, display_name)
VALUES (1, 'none', 'Nicht vorhanden'),
       (2, 'partial', 'Teilweise'),
       (3, 'substantial', 'Weitgehend'),
       (4, 'complete', 'Vollständig');

CREATE TABLE document_flashcard_statuses
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO document_flashcard_statuses (id, name, display_name)
VALUES (1, 'pending', 'Ausstehend'),
       (2, 'partially_completed', 'Teilweise abgeschlossen'),
       (3, 'completed', 'Abgeschlossen');

CREATE TABLE document_verification_statuses
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO document_verification_statuses (id, name, display_name)
VALUES (1, 'unverified', 'Nicht verifiziert'),
       (2, 'partially_verified', 'Teilweise verifiziert'),
       (3, 'verified', 'Verifiziert');

CREATE TABLE document_verification_levels
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO document_verification_levels (id, name, display_name)
VALUES (1, 'self_reviewed', 'Selbst überprüft'),
       (2, 'cross_checked', 'Quellenabgleich'),
       (3, 'validated', 'Validiert');
--endregion

--region Knotenpunkte
CREATE TABLE domains
(
    id    integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    name  varchar(50) NOT NULL UNIQUE,
    color varchar(7),
    icon  bytea
);
INSERT INTO domains (name)
VALUES ('Anderes'),
       ('Mathematik'),
       ('Informatik und Informationswissenschaft'),
       ('Physik'),
       ('Chemie'),
       ('Biologie'),
       ('Astronomie und Astrophysik'),
       ('Geowissenschaften'),
       ('Umweltwissenschaften'),
       ('Medizin und Gesundheitswissenschaften'),
       ('Technologie und Ingenieurwesen'),
       ('Psychologie'),
       ('Sozialwissenschaften'),
       ('Wirtschaftswissenschaften'),
       ('Philosophie'),
       ('Religion'),
       ('Geografie'),
       ('Kunst'),
       ('Sport und Bewegung'),
       ('Geschichte'),
       ('Sprache und Kommunikation');

CREATE TABLE domain_nodes
(
    entity_id          integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    domain_id          integer      NOT NULL REFERENCES domains (id) ON DELETE CASCADE,
    parent_id          integer REFERENCES domain_nodes (entity_id) ON DELETE CASCADE,
    knowledge_level_id integer REFERENCES knowledge_levels (id),

    name               varchar(150) NOT NULL,
    is_system_value    boolean      NOT NULL,
    last_reviewed_at   timestamptz  NOT NULL DEFAULT NOW(),

    CHECK (entity_id <> parent_id),
    UNIQUE NULLS NOT DISTINCT (domain_id, parent_id, name)
);
CREATE INDEX idx_domain_nodes_domain_parent
    ON domain_nodes (domain_id, parent_id);
CREATE TABLE attachments
(
    entity_id      integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    domain_node_id integer NOT NULL REFERENCES domain_nodes (entity_id) ON DELETE CASCADE,
    original_name  text   NOT NULL,
    storage_name   text   NOT NULL,
    size_bytes     bigint NOT NULL
);
CREATE TABLE documents
(
    domain_node_id         integer PRIMARY KEY REFERENCES domain_nodes (entity_id) ON DELETE CASCADE,

    content                text NOT NULL DEFAULT '',
    completeness_level_id  integer REFERENCES document_completeness_levels (id),
    flashcard_status_id    integer REFERENCES document_flashcard_statuses (id),
    verification_status_id integer REFERENCES document_verification_statuses (id),
    verification_level_id  integer REFERENCES document_verification_levels (id)
);
CREATE TABLE document_versions
(
    domain_node_id    integer     NOT NULL REFERENCES documents (domain_node_id) ON DELETE CASCADE,
    version           integer     NOT NULL,
    content           text        NOT NULL,
    window_started_at timestamptz NOT NULL DEFAULT NOW(),
    captured_at       timestamptz NOT NULL DEFAULT NOW(),

    PRIMARY KEY (domain_node_id, version)
);
--endregion

--region Lernkarten
CREATE TABLE flashcard_retention_classes
(
    id                integer PRIMARY KEY,
    name              varchar(50)   NOT NULL UNIQUE,
    display_name      varchar(50)   NOT NULL UNIQUE,
    desired_retention numeric(4, 3) NOT NULL,

    CHECK (desired_retention BETWEEN 0 AND 1)
);
INSERT INTO flashcard_retention_classes (id, name, display_name, desired_retention)
VALUES (1, 'supplementary', 'Zusatzwissen', 0.800),
       (2, 'general', 'Allgemeinwissen', 0.850),
       (3, 'specialized', 'Fachwissen', 0.900),
       (4, 'fundamental', 'Kernwissen', 0.930),
       (5, 'essential', 'Unverzichtbares Wissen', 0.950);

CREATE TABLE flashcard_review_states
(
    id   integer PRIMARY KEY,
    name varchar(30) NOT NULL UNIQUE
);

INSERT INTO flashcard_review_states (id, name)
VALUES (1, 'new'),
       (2, 'learning'),
       (3, 'review'),
       (4, 'relearning');

CREATE TABLE flashcards
(
    entity_id          integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    domain_node_id     integer NOT NULL REFERENCES domain_nodes (entity_id) ON DELETE CASCADE,

    front              text    NOT NULL,
    back               text    NOT NULL,
    hint               text,
    explanation        text,

    review_state_id    integer NOT NULL DEFAULT 1 REFERENCES flashcard_review_states (id),
    retention_class_id integer NOT NULL REFERENCES flashcard_retention_classes (id),

    stability          real,
    difficulty         real,

    last_reviewed_at   timestamptz,
    due_at             timestamptz,

    CHECK (stability > 0),

    CHECK (difficulty BETWEEN 1 AND 10)
);
CREATE INDEX idx_flashcards_due_at
    ON flashcards (due_at);

CREATE TABLE flashcard_reviews
(
    id                     integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    flashcard_id           integer     NOT NULL REFERENCES flashcards (entity_id) ON DELETE CASCADE,

    rating                 smallint    NOT NULL,
    stability              real        NOT NULL,
    difficulty             real        NOT NULL,
    review_state_before_id integer     NOT NULL REFERENCES flashcard_review_states (id),
    review_state_after_id  integer     NOT NULL REFERENCES flashcard_review_states (id),

    reviewed_at            timestamptz NOT NULL DEFAULT now(),
    duration_ms            integer     NOT NULL,
    next_due_at            timestamptz NOT NULL,

    CHECK (rating BETWEEN 1 AND 4),

    CHECK (stability > 0),

    CHECK (difficulty BETWEEN 1 AND 10),

    CHECK (duration_ms >= 0),

    CHECK (next_due_at > reviewed_at)
);
CREATE INDEX idx_flashcard_reviews_card_time
    ON flashcard_reviews (flashcard_id, reviewed_at DESC, id DESC);
--endregion

--region Progressionen
CREATE TABLE progressions
(
    entity_id             integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    name                  varchar(150) NOT NULL,

    personal_relevance_id integer REFERENCES personal_relevance_levels (id),
    lasting_impact_id     integer REFERENCES lasting_impacts (id),

    motivation            text,
    desired_outcome       text
);
CREATE TABLE progression_stages
(
    id             integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    progression_id integer NOT NULL REFERENCES progressions (entity_id) ON DELETE CASCADE,
    stage          integer NOT NULL,

    name           varchar(150),
    description    text,

    is_completed   bool    NOT NULL DEFAULT FALSE,
    completed_at   timestamptz,

    UNIQUE (progression_id, stage),
    CHECK (stage > 0)
);
--endregion Progressionen

--region Bestrebungen
CREATE TABLE pursuit_requirement_types
(
    id   integer PRIMARY KEY,
    name varchar(50) NOT NULL UNIQUE
);
INSERT INTO pursuit_requirement_types (id, name)
VALUES (1, 'knowledge_level'),
       (3, 'completeness_level'),
       (4, 'flashcard_status'),
       (5, 'verification_status'),
       (6, 'verification_level');

CREATE TABLE pursuits
(
    entity_id            integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    domain_node_id       integer      NOT NULL REFERENCES domain_nodes (entity_id) ON DELETE CASCADE,
    progression_stage_id integer REFERENCES progression_stages (id),

    name                 varchar(150) NOT NULL,
    achieved_at          timestamptz
);
CREATE TABLE pursuit_requirements
(
    id                     integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    pursuit_id             integer NOT NULL REFERENCES pursuits (entity_id) ON DELETE CASCADE,

    requirement_type_id    integer NOT NULL REFERENCES pursuit_requirement_types (id),

    knowledge_level_id     integer REFERENCES knowledge_levels (id),
    completeness_level_id  integer,
    flashcard_status_id    integer,
    verification_status_id integer,
    verification_level_id  integer,

    CONSTRAINT pursuit_requirements_type_check CHECK (
        num_nonnulls(
                knowledge_level_id,
                completeness_level_id,
                flashcard_status_id,
                verification_status_id,
                verification_level_id
        ) = 1
            AND ((requirement_type_id = 1 AND knowledge_level_id IS NOT NULL)
            OR (requirement_type_id = 3 AND completeness_level_id IS NOT NULL)
            OR (requirement_type_id = 4 AND flashcard_status_id IS NOT NULL)
            OR (requirement_type_id = 5 AND verification_status_id IS NOT NULL)
            OR (requirement_type_id = 6 AND verification_level_id IS NOT NULL)
            )
        ),

    UNIQUE (pursuit_id, requirement_type_id)
);
--endregion

--endregion Verwirklichung

--region Systemaufgaben

--region Metadaten der Systemtaufgaben
CREATE TABLE systemtask_groups
(
    id   integer PRIMARY KEY,
    name varchar(50) NOT NULL UNIQUE
);
INSERT INTO systemtask_groups (id, name)
VALUES (1, 'Persönliches'),
       (2, 'Einkauf'),
       (3, 'Schule');

CREATE TABLE systemtask_states
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50)
);
INSERT INTO systemtask_states (id, name, display_name)
VALUES (1, 'open', 'Offen'),
       (2, 'in_progress', 'In Bearbeitung'),
       (3, 'completed', 'Abgeschlossen'),
       (4, 'cancelled', 'Abgebrochen');

CREATE TABLE systemtask_priorities
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50)
);
INSERT INTO systemtask_priorities (id, name, display_name)
VALUES (1, 'low', 'Niedrig'),
       (2, 'normal', 'Normal'),
       (3, 'high', 'Hoch'),
       (4, 'critical', 'Kritisch');
--endregion

CREATE TABLE systemtasks
(
    entity_id                integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    domain_node_id           integer REFERENCES domain_nodes (entity_id) ON DELETE CASCADE,
    group_id                 integer REFERENCES systemtask_groups (id),

    name                     varchar(150) NOT NULL,
    description              text,
    motivation               text,
    desired_outcome          text,

    state_id                 integer      NOT NULL REFERENCES systemtask_states (id),
    priority_id              integer      NOT NULL REFERENCES systemtask_priorities (id),

    personal_relevance_id    integer REFERENCES personal_relevance_levels (id),
    lasting_impact_id        integer REFERENCES lasting_impacts (id),
    estimated_effort_minutes integer,

    due_at                   timestamptz,

    CHECK (estimated_effort_minutes > 0)
);
CREATE INDEX idx_systemtasks_domain_node
    ON systemtasks (domain_node_id);

--region Routinen der Systemaufgaben
CREATE TABLE systemtask_routines
(
    id                  integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    systemtask_id       integer UNIQUE NOT NULL REFERENCES systemtasks (entity_id) ON DELETE CASCADE,

    recurrence_rule_set text           NOT NULL,
    timezone            varchar(50)    NOT NULL DEFAULT 'Europe/Berlin',
    is_optional         boolean        NOT NULL DEFAULT false,
    is_active           boolean        NOT NULL DEFAULT true
);

CREATE TABLE systemtask_routine_execution_states
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO systemtask_routine_execution_states (id, name, display_name)
VALUES (1, 'pending', 'Ausstehend'),
       (2, 'completed', 'Abgeschlossen'),
       (3, 'skipped', 'Übersprungen'),
       (4, 'expired', 'Abgelaufen');

CREATE TABLE systemtask_routine_executions
(
    id            integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    routine_id    integer NOT NULL REFERENCES systemtask_routines (id) ON DELETE CASCADE,

    state_id      integer NOT NULL REFERENCES systemtask_routine_execution_states (id),
    is_optional   boolean NOT NULL,

    occurrence_on date    NOT NULL,
    scheduled_at  timestamptz,

    UNIQUE (routine_id, occurrence_on)
);

--endregion Routine

--region Kalender
CREATE TABLE calendar_entries
(
    entity_id            integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    systemtask_id        integer REFERENCES systemtasks (entity_id) ON DELETE CASCADE,
    routine_execution_id integer REFERENCES systemtask_routine_executions (id) ON DELETE CASCADE,

    name                 varchar(150) NOT NULL,
    note                 text,

    start_at             timestamptz  NOT NULL,
    end_at               timestamptz,
    is_locked            boolean      NOT NULL,

    CHECK (end_at IS NULL OR end_at > start_at),
    CHECK (num_nonnulls(systemtask_id, routine_execution_id) <= 1)
);
--endregion

--region Sessions der Systemaufgaben
CREATE TABLE systemtask_session_presets
(
    id                       integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    name                     varchar(100) NOT NULL UNIQUE,
    planned_duration_minutes integer      NOT NULL,

    CHECK (planned_duration_minutes > 0)
);
INSERT INTO systemtask_session_presets (name, planned_duration_minutes)
VALUES ('Kurzfokus', 15),
       ('Pomodoro', 25),
       ('Standard', 45),
       ('Fokus', 60),
       ('Deep Work', 90),
       ('Intensivarbeit', 120);

CREATE TABLE systemtask_session_states
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO systemtask_session_states (id, name, display_name)
VALUES (1, 'in_progress', 'In Bearbeitung'),
       (2, 'paused', 'Pausiert'),
       (3, 'completed', 'Abgeschlossen'),
       (4, 'cancelled', 'Abgebrochen');

CREATE TABLE systemtask_sessions
(
    id                       integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    systemtask_id            integer REFERENCES systemtasks (entity_id),
    routine_execution_id     integer REFERENCES systemtask_routine_executions (id),
    calendar_entry_id        integer     REFERENCES calendar_entries (entity_id) ON DELETE SET NULL,

    note                     text,
    state_id                 integer     NOT NULL REFERENCES systemtask_session_states (id),

    planned_duration_minutes integer,
    started_at               timestamptz NOT NULL DEFAULT NOW(),
    ended_at                 timestamptz,

    CHECK (planned_duration_minutes IS NULL OR planned_duration_minutes > 0),
    CHECK (num_nonnulls(systemtask_id, routine_execution_id) = 1),
    CHECK (ended_at IS NULL OR ended_at > started_at)
);
CREATE TABLE systemtask_session_segments
(
    id         integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
    session_id integer     NOT NULL REFERENCES systemtask_sessions (id) ON DELETE CASCADE,

    started_at timestamptz NOT NULL DEFAULT NOW(),
    ended_at   timestamptz,

    CHECK (ended_at IS NULL OR ended_at > started_at)
);
CREATE UNIQUE INDEX unique_active_session_segment
    ON systemtask_session_segments (session_id) WHERE ended_at IS NULL;
--endregion Sessions

--endregion Systemaufgaben

--region RSS
CREATE TABLE rss_categories
(
    id    integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name  varchar(100) NOT NULL UNIQUE,
    icon  bytea,
    color varchar(7)   NOT NULL
);

CREATE TABLE rss_feeds
(
    id              integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    category_id     integer REFERENCES rss_categories (id) ON DELETE SET NULL,
    url             text        NOT NULL UNIQUE,
    title           text        NOT NULL,
    last_fetched_at timestamptz,
    enabled         boolean     NOT NULL DEFAULT TRUE,
    is_hidden       boolean     NOT NULL DEFAULT FALSE,
    created_at      timestamptz NOT NULL DEFAULT NOW()
);

CREATE TABLE rss_entries
(
    id           integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    feed_id      integer     NOT NULL REFERENCES rss_feeds (id),
    external_id  text        NOT NULL,
    title        text        NOT NULL,
    url          text,
    summary      text,
    published_at timestamptz,
    is_read      boolean     NOT NULL DEFAULT FALSE,
    created_at   timestamptz NOT NULL DEFAULT NOW(),

    UNIQUE (feed_id, external_id)
);
CREATE INDEX idx_rss_entries_feed_published
    ON rss_entries (feed_id, published_at DESC);
--endregion RSS

--region Media
CREATE TABLE media_types
(
    id   integer PRIMARY KEY,
    name varchar(50) NOT NULL UNIQUE
);
INSERT INTO media_types (id, name)
VALUES (1, 'movie'),
       (2, 'tv');

CREATE TABLE media_watch_statuses
(
    id   integer PRIMARY KEY,
    name varchar(50) NOT NULL UNIQUE
);
INSERT INTO media_watch_statuses (id, name)
VALUES (1, 'planned'),
       (2, 'watching'),
       (3, 'completed'),
       (4, 'paused'),
       (5, 'dropped');

CREATE TABLE media_ratings
(
    id           integer PRIMARY KEY,
    name         varchar(50) NOT NULL UNIQUE,
    display_name varchar(50) NOT NULL UNIQUE
);
INSERT INTO media_ratings (id, name, display_name)
VALUES (1, 'disappointing', 'Enttäuschend'),
       (2, 'average', 'Durchschnittlich'),
       (3, 'worth_watching', 'Sehenswert'),
       (4, 'outstanding', 'Herausragend');

CREATE TABLE media
(
    entity_id         integer PRIMARY KEY REFERENCES entities (id) ON DELETE CASCADE,
    tmdb_id           integer NOT NULL CHECK (tmdb_id > 0),
    media_type_id     integer NOT NULL REFERENCES media_types (id),
    title             text,
    original_title    text,
    description       text,
    poster_path       text,
    backdrop_path     text,
    trailer_url       text,
    additional_backdrop_paths text[] NOT NULL DEFAULT '{}'
        CHECK (cardinality(additional_backdrop_paths) <= 4),
    release_date      date,
    runtime_minutes   integer CHECK (runtime_minutes > 0),
    production_status text,
    tmdb_vote_average double precision CHECK (tmdb_vote_average BETWEEN 0 AND 10),
    tmdb_vote_count   integer CHECK (tmdb_vote_count >= 0),
    personal_rating   integer REFERENCES media_ratings (id),
    personal_notes    text,
    watch_status_id   integer NOT NULL DEFAULT 1 REFERENCES media_watch_statuses (id),
    watch_count       integer NOT NULL DEFAULT 0 CHECK (watch_count >= 0),
    completed_at      timestamptz,
    last_synced_at    timestamptz NOT NULL DEFAULT NOW(),

    UNIQUE (tmdb_id, media_type_id),
    CHECK (runtime_minutes IS NULL OR media_type_id = 1),
    CHECK ((watch_status_id = 3) = (completed_at IS NOT NULL))
);

CREATE TABLE media_seasons
(
    id            integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    media_id      integer NOT NULL REFERENCES media (entity_id) ON DELETE CASCADE,
    tmdb_id       integer NOT NULL UNIQUE CHECK (tmdb_id > 0),
    season_number integer NOT NULL CHECK (season_number >= 0),
    title         text,
    description   text,
    poster_path   text,
    release_date  date
);
CREATE INDEX idx_media_seasons_media_number ON media_seasons (media_id, season_number);

CREATE TABLE media_episodes
(
    id              integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    season_id       integer NOT NULL REFERENCES media_seasons (id) ON DELETE CASCADE,
    tmdb_id         integer NOT NULL UNIQUE CHECK (tmdb_id > 0),
    episode_number  integer NOT NULL CHECK (episode_number >= 0),
    title           text,
    description     text,
    still_path      text,
    runtime_minutes integer CHECK (runtime_minutes > 0),
    release_date    date,
    is_watched      boolean NOT NULL DEFAULT FALSE,
    watched_at      timestamptz,

    CONSTRAINT media_episodes_watched_at_check CHECK (is_watched = (watched_at IS NOT NULL))
);
CREATE INDEX idx_media_episodes_season_number ON media_episodes (season_id, episode_number);

CREATE TABLE media_genres
(
    id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name text NOT NULL UNIQUE
);

CREATE TABLE media_genre_assignments
(
    media_id integer NOT NULL REFERENCES media (entity_id) ON DELETE CASCADE,
    genre_id integer NOT NULL REFERENCES media_genres (id),

    PRIMARY KEY (media_id, genre_id)
);
CREATE INDEX idx_media_genre_assignments_genre_id ON media_genre_assignments (genre_id);

CREATE TABLE media_keywords
(
    id   integer PRIMARY KEY CHECK (id > 0),
    name text NOT NULL
);

CREATE TABLE media_keyword_assignments
(
    media_id   integer NOT NULL REFERENCES media (entity_id) ON DELETE CASCADE,
    keyword_id integer NOT NULL REFERENCES media_keywords (id),

    PRIMARY KEY (media_id, keyword_id)
);
CREATE INDEX idx_media_keyword_assignments_keyword_id ON media_keyword_assignments (keyword_id);
--endregion Media

--region Sources
-- Zotero itemTypes und deutsche Anzeigenamen aus dem Translation-Server-Schema.
CREATE TABLE resource_types
(
    id           smallint PRIMARY KEY,
    name         text NOT NULL UNIQUE,
    display_name text NOT NULL
);
INSERT INTO resource_types (id, name, display_name)
VALUES (1, 'annotation', 'Anmerkung'),
       (2, 'artwork', 'Kunstwerk'),
       (3, 'attachment', 'Anhang'),
       (4, 'audioRecording', 'Tonaufnahme'),
       (5, 'bill', 'Gesetzentwurf'),
       (6, 'blogPost', 'Blog-Post'),
       (7, 'book', 'Buch'),
       (8, 'bookSection', 'Buchteil'),
       (9, 'case', 'Gerichtsentscheidung'),
       (10, 'computerProgram', 'Software'),
       (11, 'conferencePaper', 'Konferenz-Paper'),
       (12, 'dataset', 'Datensatz'),
       (13, 'dictionaryEntry', 'Wörterbucheintrag'),
       (14, 'document', 'Dokument'),
       (15, 'email', 'E-Mail'),
       (16, 'encyclopediaArticle', 'Enzyklopädieartikel'),
       (17, 'film', 'Film'),
       (18, 'forumPost', 'Foren-Eintrag'),
       (19, 'hearing', 'Anhörung'),
       (20, 'instantMessage', 'Instant-Message'),
       (21, 'interview', 'Interview'),
       (22, 'journalArticle', 'Zeitschriftenartikel'),
       (23, 'letter', 'Brief'),
       (24, 'magazineArticle', 'Magazin-Artikel'),
       (25, 'manuscript', 'Manuskript'),
       (26, 'map', 'Karte'),
       (27, 'newspaperArticle', 'Zeitungsartikel'),
       (28, 'note', 'Notiz'),
       (29, 'patent', 'Patent'),
       (30, 'podcast', 'Podcast'),
       (31, 'preprint', 'Preprint'),
       (32, 'presentation', 'Vortrag'),
       (33, 'radioBroadcast', 'Radiosendung'),
       (34, 'report', 'Bericht'),
       (35, 'standard', 'Norm'),
       (36, 'statute', 'Gesetz'),
       (37, 'thesis', 'Dissertation'),
       (38, 'tvBroadcast', 'Fernsehsendung'),
       (39, 'videoRecording', 'Videoaufnahme'),
       (40, 'webpage', 'Webseite');

CREATE TABLE source_categories
(
    id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name text NOT NULL UNIQUE
);

CREATE TABLE sources
(
    id               uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    resource_type_id smallint NOT NULL REFERENCES resource_types (id),
    title            text,
    url              text NOT NULL,
    description      text,
    thumbnail_url    text,
    metadata         jsonb NOT NULL,
    created_at       timestamptz NOT NULL DEFAULT now(),
    updated_at       timestamptz NOT NULL DEFAULT now(),
    category_id      integer REFERENCES source_categories (id) ON DELETE SET NULL,
    favicon          bytea,
    favicon_mime_type text
);
--endregion Sources
