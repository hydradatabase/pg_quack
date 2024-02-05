MODULE_big = quack
EXTENSION = quack
DATA = quack.control $(wildcard quack--*.sql)
PGFILEDESC = "quack"

SRCS = src/quack_hooks.o \
	   src/quack_tableam.o \
	   src/quack_utility.o \
	   src/quack_write_manager.o \
	   src/quack.o

OBJS = $(subst .c,.o, $(SRCS))

ifdef PG_CONFIG_PATH
PG_CONFIG= $(PG_CONFIG_PATH)
else
PG_CONFIG = pg_config
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
PG_LIB := $(shell $(PG_CONFIG) --pkglibdir)
INCLUDEDIR := ${shell $(PG_CONFIG) --includedir}
INCLUDEDIR_SERVER := ${shell $(PG_CONFIG) --includedir-server}

DUCKDB_DIR = third_party/duckdb

override PG_CFLAGS += -I$(CURDIR)/include -I$(CURDIR)/$(DUCKDB_DIR)

SHLIB_LINK += -Wl,-rpath,$(PG_LIB)/ -L$(PG_LIB) -lduckdb -L$(CURDIR)/$(DUCKDB_DIR)

COMPILE.c.bc = $(CLANG) -Wno-ignored-attributes -Wno-register $(BITCODE_CXXFLAGS) $(PG_CFLAGS) -I$(INCLUDEDIR_SERVER) -emit-llvm -c

%.bc : %.c
	$(COMPILE.c.bc) $(SHLIB_LINK) $(PG_CFLAGS) -I$(INCLUDE_SERVER) -o $@ $<

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  DUCKDB_LIB = libduckdb.dylib
  DUCKDB_ZIP = libduckdb-osx-universal.zip
endif
ifeq ($(UNAME_S),Linux)
  DUCKDB_LIB = libduckdb.so
  UNAME_M := $(shell uname -m)
  ifeq ($(UNAME_M),aarch64)
    DUCKDB_ZIP = libduckdb-linux-aarch64.zip
  endif
  ifeq ($(UNAME_M),x86_64)
    DUCKDB_ZIP = libduckdb-linux-amd64.zip
  endif
endif

DUCKDB_VERSION ?= v0.9.2
DUCKDB_BASE_URL ?= https://github.com/duckdb/duckdb/releases/download/$(DUCKDB_VERSION)
REGRESS =

include $(PGXS)

all: quack.so

$(DUCKDB_DIR)/$(DUCKDB_ZIP):
	curl -sL $(DUCKDB_BASE_URL)/$(DUCKDB_ZIP) -o $(DUCKDB_DIR)/$(DUCKDB_ZIP)

# this really likes to run everytime, not sure why
$(DUCKDB_DIR)/$(DUCKDB_LIB): $(DUCKDB_DIR)/$(DUCKDB_ZIP)
	cd $(DUCKDB_DIR) && unzip -o -q $(DUCKDB_ZIP) $(DUCKDB_LIB)

.PHONY: install_duckdb
install_duckdb:
	$(install_bin) -m 755 $(DUCKDB_DIR)/$(DUCKDB_LIB) $(DESTDIR)$(PG_LIB)

quack.so: $(DUCKDB_DIR)/$(DUCKDB_LIB)

install: install_duckdb
