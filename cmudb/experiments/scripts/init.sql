-- This file is meant to be run once to setup all the required tables.
CREATE TABLE IF NOT EXISTS bool_knob_changelog (
  timestamp TIMESTAMP, 
  conn_id INT, 
  knob VARCHAR, 
  new_value BOOL, 
  PRIMARY KEY(timestamp, conn_id)
);