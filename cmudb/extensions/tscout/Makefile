EXTENSION = tscout_extension
MODULE_big = tscout_extension
DATA = tscout_extension--0.0.1.sql
OBJS = main.o
# TODO: (Karthik) Add logic to fetch pg_config binary path in deployment system
PG_CONFIG = /home/kramana2/postgres/src/bin/pg_config/pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)