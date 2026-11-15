-- Add migration script here
ALTER TABLE systemtask_routine_executions
    ADD CONSTRAINT systemtask_routine_executions_routine_occurrence_key
        UNIQUE (routine_id, occurrence_on);