UPDATE ideas SET phase_id = 3 WHERE phase_id = 4;

UPDATE entity_field_versions SET old_value = '3'::jsonb
WHERE field_id = 19 AND old_value = '4'::jsonb;
UPDATE entity_field_versions SET new_value = '3'::jsonb
WHERE field_id = 19 AND new_value = '4'::jsonb;

DELETE FROM idea_phases WHERE id = 4;

UPDATE idea_phases SET name = 'Vormerkung' WHERE id = 1;
UPDATE idea_phases SET name = 'Reifung' WHERE id = 2;
UPDATE idea_phases SET name = 'Ausarbeitung' WHERE id = 3;
UPDATE idea_phases SET name = 'Überprüfung' WHERE id = 5;

ALTER TABLE idea_phases DROP COLUMN display_name, DROP COLUMN description;
