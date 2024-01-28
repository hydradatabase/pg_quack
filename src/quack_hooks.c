#include "postgres.h"

#include "miscadmin.h"

#include "access/tableam.h"
#include "executor/executor.h"
#include "parser/parse_type.h"
#include "tcop/utility.h"

#include "quack.h"

static ExecutorRun_hook_type PrevExecutorRunHook = NULL;
static ProcessUtility_hook_type PrevProcessUtilityHook = NULL;

static bool
quack_check_tables(List * rtable)
{
  ListCell *lc;
  foreach(lc, rtable)
  {
    RangeTblEntry * table = lfirst(lc);
    Relation rel = NULL;
    
    if (!table->relid)
        return false;

    rel = RelationIdGetRelation(table->relid);

    if (rel->rd_amhandler != 0 &&
        GetTableAmRoutine(rel->rd_amhandler) != quack_get_table_am_routine())
    {
      RelationClose(rel);
      return false;
    }
    RelationClose(rel);
  }

  return true;
}

static void
quack_executor_run(QueryDesc * queryDesc,
                   ScanDirection direction,
                   uint64 count,
                   bool execute_once)
{
  if (queryDesc->operation == CMD_SELECT &&
      quack_check_tables(queryDesc->plannedstmt->rtable))
  {
    duckdb_database db = quack_open_database(MyDatabaseId, false);
    duckdb_connection connection = quack_open_connection(db);
    duckdb_result result;
    idx_t row_count;
    idx_t column_count;

    CmdType operation;
    DestReceiver *dest;

    TupleTableSlot * slot = NULL;

    if(duckdb_query(connection, queryDesc->sourceText, &result) == DuckDBError)
    {
    
    }

    operation = queryDesc->operation;
    dest = queryDesc->dest;

    dest->rStartup(dest, operation, queryDesc->tupDesc);

    slot = MakeTupleTableSlot(queryDesc->tupDesc, &TTSOpsHeapTuple);

    row_count = duckdb_row_count(&result);
    column_count = duckdb_column_count(&result);

    for(idx_t row = 0; row < row_count; row++)
    {
      ExecClearTuple(slot);

      for(idx_t col = 0; col < column_count; col++)
      {
        if (duckdb_value_is_null(&result, col, row))
        {
          slot->tts_isnull[col] = true;
        }
        else
        {
          slot->tts_isnull[col] = false;
          quack_read_result(slot, &result, col, row);
        }
      }

      ExecStoreVirtualTuple(slot);
      dest->receiveSlot(slot, dest);

      for(idx_t i = 0; i < column_count; i++)
      {
        if (slot->tts_tupleDescriptor->attrs[i].attbyval == false)
        {
          pfree(DatumGetPointer(slot->tts_values[i]));
        }
      }
    }

    dest->rShutdown(dest);

    duckdb_destroy_result(&result);
    duckdb_disconnect(&connection);
    duckdb_close(&db);

    return;
  }

  if (PrevExecutorRunHook)
  {
    PrevExecutorRunHook(queryDesc, direction, count, execute_once);
  }
}

static void 
quack_process_utility(PlannedStmt * pstmt,
                      const char * queryString,
                      bool readOnlyTree,
                      ProcessUtilityContext context,
                      ParamListInfo params,
                      struct QueryEnvironment * queryEnv,
                      DestReceiver * dest,
                      QueryCompletion * completionTag)
{

  Node *parsetree = pstmt->utilityStmt;

  if (IsA(parsetree, CreateStmt))
  {
    CreateStmt * create_stmt = (CreateStmt *) parsetree;
    ListCell * lc;

    if (create_stmt->accessMethod && 
        !memcmp(create_stmt->accessMethod, "quack", 5))
    {
      StringInfo create_table_str = makeStringInfo();
      bool first = true;
      appendStringInfo(create_table_str, "CREATE TABLE %s (", create_stmt->relation->relname);
      foreach(lc, create_stmt->tableElts)
      {
        ColumnDef * def = lfirst(lc);
        Oid pg_oid = LookupTypeNameOid(NULL, def->typeName, true);

        if (first)
        {
          first = false;
        }
        else
        {
          appendStringInfo(create_table_str, ", ");
        }

        appendStringInfo(create_table_str, "%s %s", def->colname, quack_duckdb_type(pg_oid));
      }
      appendStringInfo(create_table_str, ");");

      quack_execute_query(create_table_str->data);
    }
  }

  PrevProcessUtilityHook(pstmt, queryString, false, context,
                         params, queryEnv, dest, completionTag);
}

void
quack_init_hooks(void)
{
  PrevExecutorRunHook = ExecutorRun_hook ? ExecutorRun_hook : standard_ExecutorRun;
  ExecutorRun_hook = quack_executor_run;

  PrevProcessUtilityHook = ProcessUtility_hook ? ProcessUtility_hook : standard_ProcessUtility;
  ProcessUtility_hook = quack_process_utility;
}