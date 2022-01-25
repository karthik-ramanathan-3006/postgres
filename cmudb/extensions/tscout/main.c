#include <time.h>

#include "postgres.h"
#include "fmgr.h"
#include "commands/createas.h"
#include "commands/explain.h"
#include "optimizer/planner.h"
#include "parser/parsetree.h"
#include "utils/builtins.h"
#include "../../../src/include/tscout/marker.h"
#include "operating_unit_features.h"

PG_MODULE_MAGIC;
void _PG_init(void);
void _PG_fini(void);

static void ExplainOneQueryWrapper(Query* query, int cursorOptions, IntoClause* into,
                                   ExplainState* es, const char* queryString,
                                   ParamListInfo params, QueryEnvironment *queryEnv);
static void WalkPlan(Plan *plan, ExplainState *es);
static void explainXs(Plan *node, ExplainState *es);

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
    QueryDesc  *queryDesc;
    instr_time plan_start, plan_duration;
    int eflags = 0;

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

    TS_MARKER("TS_MARKER <NAME>");
    
    if (es->format == EXPLAIN_FORMAT_TSCOUT) {

        queryDesc = CreateQueryDesc(plan, queryString, InvalidSnapshot, InvalidSnapshot, None_Receiver, params, queryEnv, 0);

        if (es->analyze)
            eflags = 0;				/* default run-to-completion flags */
        else
            eflags = EXEC_FLAG_EXPLAIN_ONLY;
        if (into)
            eflags |= GetIntoRelEFlags(into);
        
        // Run the executor
        ExecutorStart(queryDesc, eflags);
        // This calls into initPlan() which populates the plan tree.
        // TODO: Create a hook to executor start.

        // Finally, walk through the plan, dumping its out in a separate top-level group.
        ExplainOpenGroup("TscoutProps", NULL, true, es);
        WalkPlan(queryDesc->planstate->plan, es);
        ExplainCloseGroup("TscoutProps", NULL, true, es);

        // Free the created query description resources.
        ExecutorEnd(queryDesc);
	    FreeQueryDesc(queryDesc);
    }

    // Finally, after performing the extension specific operations, explain the planner.
    ExplainOnePlan(plan, into, es, queryString, params, queryEnv, &plan_duration, NULL);
}

/**
 * @brief - Explains the features of the given node. 
 * 
 * @param node (Plan *) - Plan node to be explained.
 * @param es (ExplainState *) - The current EXPLAIN state.
 */
static void explainXs(Plan *node, ExplainState *es) {
    char *nodeTagExplainer = NULL, nodeName[17];

    // NOTE: It is assumed that ou_list contains definitions for all the node tags.
    nodeTagExplainer = ou_list[nodeTag(node)].name;
    
    sprintf(nodeName, "node-%d", node->plan_node_id);
    ExplainPropertyText("node", nodeName, es);
    ExplainPropertyText("tag", nodeTagExplainer, es);
    ExplainPropertyFloat("startup_cost", "units", (node->startup_cost), 6, es);
    ExplainPropertyFloat("total_cost", "units", (node->total_cost), 6, es);
    ExplainPropertyInteger("plan_width", "units", node->plan_width, es);
    ExplainPropertyText("X's", ou_list[nodeTag(node)].features[0], es);
}

/**
 * @brief - Walks through the plan tree.
 * 
 * @param plan (Plan *) - Plan node.
 * @param es (ExplainState *) - The current EXPLAIN state.
 */
static void WalkPlan(Plan *plan, ExplainState *es) {
    if (plan == NULL) 
        return;

    // 1. Explain the current node.
    explainXs(plan, es);

    // 2. Explain the tree rooted in the outer (left) plan.
    if (plan != NULL && outerPlan(plan) != NULL) {
        ExplainOpenGroup("left-child", "left-child", true, es);
        WalkPlan(outerPlan(plan), es);
        ExplainCloseGroup("left-child", "left-child", true, es);
    }

	// 3. Explain the tree rooted in the inner (right) plan.
    if (plan != NULL && innerPlan(plan) != NULL) {
        ExplainOpenGroup("right-child", "right-child", true, es);
        WalkPlan(innerPlan(plan), es);
        ExplainCloseGroup("right-child", "right-child", true, es);
    }
}
