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

UPDATE media
SET personal_rating = CASE
    WHEN personal_rating >= 9 THEN 4
    WHEN personal_rating >= 7 THEN 3
    WHEN personal_rating >= 5 THEN 2
    ELSE 1
END
WHERE personal_rating IS NOT NULL;

ALTER TABLE media
    DROP CONSTRAINT media_personal_rating_check,
    ADD FOREIGN KEY (personal_rating) REFERENCES media_ratings (id);
