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

SHLIB_LINK += -Wl,-rpath=$(PG_LIB)/ -L$(PG_LIB) -lduckdb

COMPILE.c.bc = $(CLANG) -Wno-ignored-attributes -Wno-register $(BITCODE_CXXFLAGS) $(PG_CFLAGS) -I$(INCLUDEDIR_SERVER) -emit-llvm -c

%.bc : %.c
	$(COMPILE.c.bc) $(SHLIB_LINK) $(PG_CFLAGS) -I$(INCLUDE_SERVER) -o $@ $<

# Platform detection.
ifeq ($(OS),Windows_NT)
	# noop for now
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
	endif
	ifeq ($(UNAME_S),Linux)
	endif
endif

REGRESS = 

include $(PGXS)

all: quack.so
