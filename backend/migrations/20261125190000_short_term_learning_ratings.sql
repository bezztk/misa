CREATE TABLE short_term_learning_card_ratings
(
    id               integer PRIMARY KEY,
    name             varchar(30) NOT NULL UNIQUE,
    display_name     text        NOT NULL,
    interval_minutes integer
);

INSERT INTO short_term_learning_card_ratings (id, name, display_name, interval_minutes)
VALUES (1, 'again', 'Nochmal', 1),
       (2, 'hard', 'Schwer', 4),
       (3, 'good', 'Gut', 10),
       (4, 'easy', 'Einfach', NULL);

ALTER TABLE short_term_learning_cards
    ADD COLUMN front_to_back_last_rating_id integer REFERENCES short_term_learning_card_ratings (id),
    ADD COLUMN front_to_back_due_at timestamptz,
    ADD COLUMN back_to_front_last_rating_id integer REFERENCES short_term_learning_card_ratings (id),
    ADD COLUMN back_to_front_due_at timestamptz;
