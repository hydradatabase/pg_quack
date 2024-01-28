#pragma once

#include "postgres.h"

#include "storage/relfilenode.h"
#include "access/tupdesc.h"
#include "access/tableam.h"
#include "utils/rel.h"

#include "duckdb.h"

/* Quack GUC */
extern char * quack_data_dir;

typedef struct QuackWriteState
{
    Oid rel_node;
    duckdb_database database;
    duckdb_connection connection;
    duckdb_appender appender;
    uint16 row_count;
} QuackWriteState;


// quack_hooks.c
extern void quack_init_hooks(void);

// quack_tableam.c
extern void quack_init_tableam(void);
const TableAmRoutine * quack_get_table_am_routine(void);

// quack_utility.c
extern void quack_execute_query(const char * query);
extern duckdb_database quack_open_database(Oid databaseOid, bool preserveInsertOrder);
extern duckdb_connection quack_open_connection(duckdb_database database);
extern duckdb_appender quack_create_appender(duckdb_connection connection,
                                             const char * tableName);
extern const char * quack_duckdb_type(Oid columnOid);
extern void quack_append_value(duckdb_appender appender, Oid columnOid, Datum value);
extern void quack_read_result(TupleTableSlot * slot, duckdb_result * result,
                              idx_t col, idx_t row);


// quack_write_manager.c
extern QuackWriteState * quack_init_write_state(Relation relation,
                                                Oid databaseOid,
                                                SubTransactionId currentSubXid);
extern void quack_flush_write_state(SubTransactionId currentSubXid,
                                    SubTransactionId parentSubXid,
                                    bool commit);
// quack.c
void _PG_init(void);
