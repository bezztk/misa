-- Add migration script here
ALTER TABLE systemtask_routine_executions
DROP COLUMN occurrence_at,
    ADD COLUMN occurrence_on DATE NOT NULL,
    ADD COLUMN scheduled_at TIMESTAMPTZ NULL;