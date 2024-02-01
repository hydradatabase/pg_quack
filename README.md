# pg_quack

pg_quack is PostgreSQL with Embedded DuckDB :duck:

## Installation

1. Visit https://duckdb.org/docs/installation/index?version=latest&environment=cplusplus&installer=binary&platform=linux
2. Fetch AMD64 or ARM64 zip file.
3. Unzip and copy shared libduckdb.so to PostgreSQL installation lib directory

```
ubuntu@ ~> pg_config --pkglibdir
/usr/lib/postgresql/14/lib
ubuntu@ ~> 
```

4. `make install`
5. Create quack directory and set permissions so PostgreSQL process can write to it. Directory can be changed
   with `quack.data_dir` configuration parameter
```
postgres=# show quack.data_dir;
 quack.data_dir 
----------------
 /opt/quack/
(1 row)

postgres=#
```

## Usage

```
CREATE TABLE quack_test (...) USING quack;
```

### Limitations

* Only COPY, INSERT, and SELECT are supported.
* Only single connection can execute INSERT and SELECT against quack table 
* Limited support for only basic data types
* You cannot query between `quack` tables and other storage methods (including Postgres `heap` tables).
