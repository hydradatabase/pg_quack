# pg_quack

pg_quack is PostgreSQL with Embedded DuckDB :duck:

## Installation

1. `make install`
1. Visit https://duckdb.org/docs/installation/index?version=latest&environment=cplusplus&installer=binary&platform=linux
1. Fetch AMD64 or ARM64 zip file.
1. Unzip and copy shared libduckdb.so to /opt/database/postgres/lib64.

## Usage

```
CREATE TABLE quack_test (...) USING quack;
```

### Limitations

* Only COPY, INSERT, and SELECT are supported.
* You cannot query between `quack` tables and other storage methods (including Postgres `heap` tables).
