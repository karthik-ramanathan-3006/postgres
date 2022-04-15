-- Drop all created event triggers
DROP EVENT TRIGGER et_pg_class_insert;

-- Create a function that captures event metadata (and logs it)
CREATE OR REPLACE FUNCTION et_log_pg_class_insert() RETURNS event_trigger AS $$
DECLARE
    obj record;
    meta varchar;
BEGIN
    -- Subtle point: TG_TAG indicates WHY the trigger was fired.
    -- pg_event_trigger_ddl_commands().object_type indicates WHAT was affected by the DDL command.
    -- For example: A "CREATE TABLE" command also creates indexes.
    -- TG_TAG fires for "CREATE TABLE", whereas pg_event_trigger_ddl_commands() contains both
    -- table and index data.
    
    IF TG_TAG LIKE 'CREATE%' OR TG_TAG LIKE 'ALTER%' THEN
        FOR obj IN SELECT * FROM pg_event_trigger_ddl_commands()
        LOOP
            IF obj.object_type = 'table' THEN
                SELECT relkind INTO meta FROM pg_class WHERE oid = obj.objid;
                INSERT INTO log_trigger(ts, cause, params) VALUES(NOW(), 'CREATE TABLE: ' || obj.object_identity, 'Table kind: ' || meta);
            END IF;

            IF obj.object_type = 'index' THEN
                SELECT tablename INTO meta FROM pg_indexes JOIN pg_class ON pg_indexes.indexname = pg_class.relname WHERE pg_class.oid = obj.objid;
                INSERT INTO log_trigger(ts, cause, params) VALUES(NOW(), 'CREATE INDEX: ' || obj.object_identity, 'Index Table: ' || meta);
            END IF;

            RAISE NOTICE 'Object Name: % Object Type: %', obj.object_identity, obj.object_type;
        END LOOP;
    END IF;
END;
$$ LANGUAGE plpgsql;

-- Create the event trigger to call into the above function.
CREATE EVENT TRIGGER et_pg_class_insert ON ddl_command_end EXECUTE FUNCTION et_log_pg_class_insert();