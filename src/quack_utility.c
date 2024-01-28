#include "postgres.h"

#include "miscadmin.h"

#include "catalog/pg_type.h"
#include "utils/fmgrprotos.h"

#include "duckdb.h"

#include "quack.h"

// DuckDB has date starting from 1/1/1970 while PG starts from 1/1/2000
#define QUACK_DUCK_DATE_OFFSET 10957
#define QUACK_DUCK_TIMESTAMP_OFFSET INT64CONST(10957) * USECS_PER_DAY


static StringInfo
quack_database_path(Oid databaseOid)
{
  StringInfo str = makeStringInfo();
  appendStringInfo(str, "%s/%u.duckdb", quack_data_dir, databaseOid);
  return str;
}

void
quack_execute_query(const char * query)
{
  duckdb_database db;
  duckdb_connection connection;

  db = quack_open_database(MyDatabaseId, true);

  if (duckdb_connect(db, &connection) == DuckDBError)
  {

  }

  duckdb_query(connection, query, NULL);
  duckdb_disconnect(&connection);
}

duckdb_database
quack_open_database(Oid databaseOid, bool preserveInsertOrder)
{
  duckdb_database database;
  duckdb_config config;

  StringInfo database_path = quack_database_path(databaseOid);
  StringInfo error_string = makeStringInfo();

  /* Set lock for relation until transaction ends */
  DirectFunctionCall1(pg_advisory_xact_lock_int8,
                      Int64GetDatum((int64) databaseOid));

  if (duckdb_create_config(&config) == DuckDBError)
  {

  }

  if (preserveInsertOrder)
    duckdb_set_config(config, "preserve_insertion_order", "false");

  enlargeStringInfo(error_string, 1024);

  if (duckdb_open_ext(database_path->data, &database, config, &error_string->data) == DuckDBError)
  {
      elog(WARNING, "[quack_open_database][duckdb_open_ext] %s", error_string->data);
  }

  pfree(error_string->data);

  duckdb_destroy_config(&config);

  return database;
}

duckdb_connection
quack_open_connection(duckdb_database database)
{
  duckdb_connection connection;

  if (duckdb_connect(database, &connection) == DuckDBError)
  {

  }

  return connection;
}

duckdb_appender
quack_create_appender(duckdb_connection connection, const char * tableName)
{
  duckdb_appender appender;

  if (duckdb_appender_create(connection, NULL, tableName, &appender) == DuckDBError)
  {

  }

  return appender;
}

const char *
quack_duckdb_type(Oid columnOid)
{
  switch(columnOid)
  {
    case BOOLOID:
      return "BOOLEAN";
    case CHAROID:
      return "TINYINT";
    case INT2OID:
      return "SMALLINT";
    case INT4OID:
      return "INTEGER";
    case INT8OID:
      return "INT8";
    case BPCHAROID:
    case TEXTOID:
    case VARCHAROID:
      return "TEXT";
    case DATEOID:
      return "DATE";
    case TIMESTAMPOID:
      return "TIMESTAMP";
    default:
      elog(ERROR, "Unsuported quack type: %d", columnOid);
  }
}

void
quack_append_value(duckdb_appender appender, Oid columnOid, Datum value)
{
  switch(columnOid)
  {
    case BOOLOID:
      duckdb_append_bool(appender, value);
      break;
    case CHAROID:
      duckdb_append_int8(appender, value);
      break;
    case INT2OID:
      duckdb_append_int16(appender, value);
      break;
    case INT4OID:
      duckdb_append_int32(appender, value);
      break;
    case INT8OID:
      duckdb_append_int64(appender, value);
      break;
    case BPCHAROID:
    case TEXTOID:
    case VARCHAROID:
    {
      const char * text = VARDATA_ANY(value);
      int len = VARSIZE_ANY_EXHDR(value);
      duckdb_append_varchar_length(appender, text, len);
      break;
    }
    case DATEOID:
    {
      duckdb_date date = { .days = value + QUACK_DUCK_DATE_OFFSET};
      duckdb_append_date(appender, date);
      break;
    }
    case TIMESTAMPOID:
    {
      duckdb_timestamp timestamp = { .micros = value + QUACK_DUCK_TIMESTAMP_OFFSET };
      duckdb_append_timestamp(appender, timestamp);
      break;
    }
    default:
      elog(ERROR, "Unsuported quack type: %d", columnOid);
  }
}

void
quack_read_result(TupleTableSlot * slot, duckdb_result * result,
                  idx_t col, idx_t row)
{
  Oid oid = slot->tts_tupleDescriptor->attrs[col].atttypid;

  switch(oid)
  {
    case BOOLOID:
      slot->tts_values[col] = duckdb_value_boolean(result, col, row);
      break;
    case CHAROID:
      slot->tts_values[col] = duckdb_value_int8(result, col, row);
      break;
    case INT2OID:
      slot->tts_values[col] = duckdb_value_int16(result, col, row);
      break;
    case INT4OID:
      slot->tts_values[col] = duckdb_value_int32(result, col, row);
      break;
    case INT8OID:
      slot->tts_values[col] = duckdb_value_int64(result, col, row);
      break;
    case BPCHAROID:
    case TEXTOID:
    case VARCHAROID:
    {
      char * varchar = duckdb_value_varchar(result, col, row);
      int varchar_len = strlen(varchar);
      text * result = palloc0(varchar_len + VARHDRSZ);
      SET_VARSIZE(result, varchar_len + VARHDRSZ);
      memcpy(VARDATA(result), varchar, varchar_len);
      slot->tts_values[col] = PointerGetDatum(result);
      duckdb_free(varchar);
      break;
    }
    case DATEOID:
    {
      duckdb_date date = duckdb_value_date(result, col, row);
      slot->tts_values[col] = date.days - QUACK_DUCK_DATE_OFFSET;
      break;
    }
    case TIMESTAMPOID:
    {
      duckdb_timestamp timestamp = duckdb_value_timestamp(result, col, row);
      slot->tts_values[col] = timestamp.micros - QUACK_DUCK_TIMESTAMP_OFFSET;
      break;
    }
    case FLOAT8OID:
    case NUMERICOID:
    {
      double result_double = duckdb_value_double(result, col, row);
      slot->tts_tupleDescriptor->attrs[col].atttypid = FLOAT8OID;
      slot->tts_tupleDescriptor->attrs[col].attbyval = true;
      memcpy(&slot->tts_values[col], (char *) &result_double, sizeof(double));
      break;
    }
    default:
        elog(ERROR, "Unsuported quack type: %d", oid);
  }
}
