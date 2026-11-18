ALTER TABLE systemtasks
    DROP CONSTRAINT systemtasks_domain_node_id_key;

ALTER TABLE systemtasks
    DROP COLUMN pursuit_id;

CREATE INDEX idx_systemtasks_domain_node
    ON systemtasks (domain_node_id);
