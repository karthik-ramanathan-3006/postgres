// clang-format off
// Extension magic.
#include "postgres.h"
#include "fmgr.h"
// clang-format on

#include "access/heapam.h"
#include "access/table.h"
#include "access/xact.h"
#include "catalog/namespace.h"
#include "utils/guc.h"
#include "utils/fmgroids.h"
#include "utils/fmgrprotos.h"

PG_MODULE_MAGIC;
void _PG_init(void);
void _PG_fini(void);

static void GucBoolAssignHookWrapper(bool newval, void *extra);
static const char* TABLE_NAME = "bool_knob_changelog";

void _PG_init(void) {
  elog(LOG, "Initializing knobber extension.");
  guc_bool_hook = GucBoolAssignHookWrapper;

  // Init logic (like setting flags) go here.
}

void _PG_fini(void) {
  elog(DEBUG1, "Finishing extension.");

  
}

static void GucBoolAssignHookWrapper(bool newval, void *extra) {
    int pid = -1;

    FunctionCallInfoBaseData *fCallArgs;
    FmgrInfo   *finfo;
    Oid table_oid;
    Datum values[4];
    bool is_null[4];
    Relation knob_change = NULL;

    HeapTuple tuple = NULL;
    // TupleDesc desc = NULL;
  
    // Initialize everything
    finfo = palloc0(sizeof(FmgrInfo));
    fCallArgs = palloc0(sizeof(FunctionCallInfoBaseData));
    memset(values, 0, sizeof(values));
    memset(is_null, 0, sizeof(is_null));

    // Populate the FMgr call info.
    fmgr_info(F_PG_BACKEND_PID, finfo);
    fCallArgs->flinfo = finfo;
    
    pid = pg_backend_pid(fCallArgs);

    table_oid = RelnameGetRelid(TABLE_NAME);
    elog(LOG, "Fetched table OID: %d", table_oid);

    // Open the table and prepare for insert
    knob_change = table_open(table_oid, RowExclusiveLock);

    values[0] = Int64GetDatumFast(GetCurrentTransactionStartTimestamp());
    values[1] = Int64GetDatumFast(pid);
    values[2] = DatumGetCString("enable_sort\0");
    values[3] = BoolGetDatum(newval);
  
    // Heapify and insert
    tuple = heap_form_tuple(knob_change->rd_att, values, is_null);
    simple_heap_insert(knob_change, tuple);

    // Free up everything
    pfree(finfo);
    pfree(fCallArgs);

    if (knob_change != NULL) {
      table_close(knob_change, RowExclusiveLock);
    }
}
