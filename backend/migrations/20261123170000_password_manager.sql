CREATE TABLE password_categories
(
    id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name varchar(100) NOT NULL UNIQUE
);

INSERT INTO password_categories (name)
VALUES ('Allgemein'),
       ('Privat'),
       ('Arbeit');

CREATE TABLE password_entries
(
    id          integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    category_id integer REFERENCES password_categories (id) ON DELETE SET NULL,
    ciphertext  bytea NOT NULL,
    salt        bytea NOT NULL,
    nonce       bytea NOT NULL
);
