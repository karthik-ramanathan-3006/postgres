### Experiments on Data Collection
* Changes to Table Schema, Indexes, Trigger and Constraints:
  * **Solution**: Creation of a trigger to identify DDL changes (INSERT/UPDATE)
  * **Writes to**: Writes metadata to a userspace table called 'log_trigger'.
  * **Artifact**: SQL script: cmudb/experiments/scripts/ddl_trigger.sql
  * TBD:
    1. Log appropriate timestamp --> transaction-start timestamp for now.
    2. Finalize the schema.

* Changes to Knobs:
  * **Solution**: A lightweight extension that plugs into Postgres' GUC.
  * **Pitfall**: Needs a small to change to Postgres' GUC, in order to expose hook.
  * **Writes to**: File currently: TBD
  * **Artifact**: Extension: cmudb/extensions/knobber
  * TBD:
    1. Log connection IDs --> Solved, not implemented.
    2. Where do we persist the data?

* Periodic sweep of stats and metrics:
  * Current status: Figuring out the most efficient way to do this.

* Capturing changes to pg_class on vacuum:
  * Current status: Brainstorming ideas

* Given a query plan, predicting when a trigger will be invoked.
  * Current status: Exploration in progress

Doc: https://docs.google.com/document/d/1xB5S14TcsTcu5y09gL2Ok-WU3Yn8ThzFr-1G9sAl5yM/edit?usp=sharing 