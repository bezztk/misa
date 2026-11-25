DELETE FROM pursuit_requirements
WHERE requirement_type_id = 2;

ALTER TABLE pursuit_requirements
    DROP COLUMN skill_id,
    DROP COLUMN psychomotor_level_id,
    ADD CONSTRAINT pursuit_requirements_type_check CHECK (
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
    ADD UNIQUE (pursuit_id, requirement_type_id);

DELETE FROM pursuit_requirement_types
WHERE id = 2;

DELETE FROM entities
WHERE type_id = 5;

DELETE FROM entity_fields
WHERE entity_type_id = 5;

DELETE FROM entity_types
WHERE id = 5;

DROP TABLE skills;
DROP TABLE psychomotor_levels;
