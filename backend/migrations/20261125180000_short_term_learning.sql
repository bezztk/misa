CREATE TABLE short_term_learning_sets
(
    id            integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    title         text        NOT NULL,
    bidirectional boolean     NOT NULL,
    created_at    timestamptz NOT NULL DEFAULT NOW()
);

CREATE TABLE short_term_learning_card_states
(
    id   integer PRIMARY KEY,
    name varchar(30) NOT NULL UNIQUE
);

INSERT INTO short_term_learning_card_states (id, name)
VALUES (1, 'new'),
       (2, 'learning'),
       (3, 'completed');

CREATE TABLE short_term_learning_cards
(
    id                     integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    set_id                 integer NOT NULL REFERENCES short_term_learning_sets (id) ON DELETE CASCADE,
    front                  text    NOT NULL,
    hint                   text,
    back                   text    NOT NULL,
    front_to_back_state_id integer NOT NULL DEFAULT 1 REFERENCES short_term_learning_card_states (id),
    back_to_front_state_id integer REFERENCES short_term_learning_card_states (id)
);

CREATE INDEX idx_short_term_learning_cards_set_id
    ON short_term_learning_cards (set_id, id);
