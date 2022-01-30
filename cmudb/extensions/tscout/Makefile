EXTENSION = tscout_extension
MODULE_big = tscout_extension
DATA = tscout_extension--0.0.1.sql
OBJS = main.o
# If PG_CONFIG is not set, try the default NoisePage build folder.
PG_CONFIG ?= ../../../build/bin/pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)