// clang-format off
// Extension magic.
#include "postgres.h"
#include "fmgr.h"
// clang-format on

#include "utils/guc.h"

PG_MODULE_MAGIC;
void _PG_init(void);
void _PG_fini(void);

static void GucBoolAssignHookWrapper(bool newval, void *extra);

void _PG_init(void) {
  elog(LOG, "Initializing knobber extension.");
  guc_bool_hook = GucBoolAssignHookWrapper;

  // Init logic (like setting flags) go here.
}

void _PG_fini(void) {
  elog(DEBUG1, "Finishing extension.");

  
}

static void GucBoolAssignHookWrapper(bool newval, void *extra) {
    elog(LOG, "Received value %x", newval);
}



