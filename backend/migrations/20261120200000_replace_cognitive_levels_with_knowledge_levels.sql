-- Bloom-Stufen und EQR-Kenntnisstufen beschreiben unterschiedliche Modelle.
-- Bestehende Bewertungen werden deshalb nicht anhand ihrer IDs übertragen.
DELETE FROM pursuit_requirements
WHERE requirement_type_id = 1;

UPDATE domain_nodes
SET cognitive_level_id = NULL
WHERE cognitive_level_id IS NOT NULL;

DELETE FROM entity_field_versions
WHERE field_id = 2;

ALTER TABLE domain_nodes
    DROP CONSTRAINT domain_nodes_cognitive_level_id_fkey;

ALTER TABLE domain_nodes
    RENAME COLUMN cognitive_level_id TO knowledge_level_id;

ALTER TABLE pursuit_requirements
    RENAME COLUMN cognitive_level_id TO knowledge_level_id;

DROP TABLE cognitive_levels;

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

ALTER TABLE domain_nodes
    ADD CONSTRAINT domain_nodes_knowledge_level_id_fkey
        FOREIGN KEY (knowledge_level_id) REFERENCES knowledge_levels (id);

ALTER TABLE pursuit_requirements
    ADD CONSTRAINT pursuit_requirements_knowledge_level_id_fkey
        FOREIGN KEY (knowledge_level_id) REFERENCES knowledge_levels (id);

UPDATE entity_fields
SET name = 'knowledge_level_id'
WHERE id = 2;

UPDATE pursuit_requirement_types
SET name = 'knowledge_level'
WHERE id = 1;
