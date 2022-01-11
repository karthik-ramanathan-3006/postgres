#include <time.h>

#include "postgres.h"
#include "fmgr.h"
#include "commands/explain.h"
#include "optimizer/planner.h"
#include "parser/parsetree.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;
void _PG_init(void);
void _PG_fini(void);

static void ExplainOneQueryWrapper(Query* query, int cursorOptions, IntoClause* into,
                                   ExplainState* es, const char* queryString,
                                   ParamListInfo params, QueryEnvironment *queryEnv);

static ExplainOneQuery_hook_type chain_ExplainOneQuery = NULL;

void _PG_init(void) {
    elog(LOG, "Initializing extension");
    
    // Chain the Extension Explain Query Wrapper to the head of the chain.
    chain_ExplainOneQuery = ExplainOneQuery_hook;
    ExplainOneQuery_hook = ExplainOneQueryWrapper;

    // Init logic (like setting flags) go here
}

void _PG_fini(void) {
    // Clean up logic goes here
    ExplainOneQuery_hook = chain_ExplainOneQuery;
    elog(DEBUG1, "Finishing extension");

}

static void ExplainOneQueryWrapper(Query* query, int cursorOptions, IntoClause* into,
                                   ExplainState* es, const char* queryString,
                                   ParamListInfo params, QueryEnvironment* queryEnv) {
    
    PlannedStmt *plan;
    instr_time plan_start, plan_duration;

    if (chain_ExplainOneQuery) {
        chain_ExplainOneQuery(query, cursorOptions, into, es,
                              queryString, params, queryEnv);
    }

    // Postgres does not expose an interface to call into the standard ExplainOneQuery.
    // Hence, we duplicate the operations performed by the standard ExplainOneQuery ie
    // calling into the standard planner.
    // A non-standard planner can be hooked in, in the the future.
    INSTR_TIME_SET_CURRENT(plan_start);
    plan = (planner_hook ? planner_hook(query, queryString, cursorOptions, params) : 
    standard_planner(query, queryString, cursorOptions, params));
    
    INSTR_TIME_SET_CURRENT(plan_duration);
    INSTR_TIME_SUBTRACT(plan_duration, plan_start);
    
    if (es->format == EXPLAIN_FORMAT_TSCOUT) {
        elog(DEBUG1, "Inside the explain one query");
        ExplainOpenGroup("TestProps", NULL, true, es);
        ExplainOpenGroup("Test", "Test", true, es);
        ExplainPropertyText("key", "Karthik was here", es);
        ExplainCloseGroup("Test", "Test", true, es);
        ExplainCloseGroup("TestProps", NULL, true, es);
    }

    // Finally, after performing the extension specific operations, explain the planner.
    ExplainOnePlan(plan, into, es, queryString, params, queryEnv, &plan_duration, NULL);
}
