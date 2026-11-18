CREATE TABLE attachments
(
    id             integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    domain_node_id integer     NOT NULL REFERENCES domain_nodes (id) ON DELETE CASCADE,
    original_name  text        NOT NULL,
    storage_name   text        NOT NULL,
    size_bytes     bigint      NOT NULL,
    created_at     timestamptz NOT NULL DEFAULT CURRENT_TIMESTAMP
);
