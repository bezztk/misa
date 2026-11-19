CREATE TABLE document_versions
(
    domain_node_id    integer     NOT NULL REFERENCES documents (domain_node_id) ON DELETE CASCADE,
    version           integer     NOT NULL,
    content           text        NOT NULL,
    window_started_at timestamptz NOT NULL DEFAULT NOW(),
    captured_at       timestamptz NOT NULL DEFAULT NOW(),

    PRIMARY KEY (domain_node_id, version)
);
