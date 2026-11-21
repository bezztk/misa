INSERT INTO entity_types (id, name)
VALUES (9, 'ideas');

INSERT INTO entity_fields (id, entity_type_id, field_type_id, name)
VALUES (17, 9, 1, 'title'),
       (18, 9, 2, 'content'),
       (19, 9, 3, 'phase_id'),
       (20, 9, 4, 'state_id');

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
