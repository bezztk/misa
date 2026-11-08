-- Add migration script here
DELETE FROM topic_understanding_levels;
DELETE FROM document_completeness_levels;
INSERT INTO topic_understanding_levels (id, name, display_name)
VALUES (1, 'none', 'Kein Verständnis'),
       (2, 'basic', 'Grundlegend'),
       (3, 'developing', 'Im Aufbau'),
       (4, 'proficient', 'Sicher'),
       (5, 'advanced', 'Fortgeschritten'),
       (6, 'mastered', 'Gemeistert');
INSERT INTO document_completeness_levels (id, name, display_name)
VALUES (1, 'none', 'Keine Inhalte'),
       (2, 'partial', 'Teilweise vollständig'),
       (3, 'substantial', 'Weitgehend vollständig'),
       (4, 'complete', 'Vollständig');