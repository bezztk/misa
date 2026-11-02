-- Add migration script here
CREATE TABLE items (
   id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
   type INTEGER NOT NULL REFERENCES types(id),
   created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
   updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
   is_archived BOOLEAN NOT NULL DEFAULT FALSE,
   version INTEGER NOT NULL DEFAULT 1
);

CREATE FUNCTION update_item_metadata()
    RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    NEW.version = OLD.version + 1;
RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_update_item_metadata
    BEFORE UPDATE ON items
    FOR EACH ROW
    EXECUTE FUNCTION update_item_metadata();