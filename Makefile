# === Config ===
CC := gcc
# todo: re-add -pedantic when we get C23 support
# -Wmissing-prototypes
CFLAGS := -std=gnu2x -Wall -Wextra \
         -Wcast-qual -Wcast-align -Wstrict-aliasing -Wpointer-arith \
         -Winit-self -Wshadow -Wstrict-prototypes -Wformat -Wno-format-zero-length \
         -Wredundant-decls -Wfloat-equal -Wundef -Wvla -Wno-parentheses \
         -Ilib/own -isystem lib/vendor \
		 -Werror=incompatible-pointer-types \
         -D__SKIP_GNU \
 		 #-fsanitize=address # messes with debugging

LFLAGS_CLIENT := -I/usr/include/json-c -ljson-c
LFLAGS_SERVER := -I/usr/include/json-c -ljson-c -I/usr/include/postgresql -L/usr/lib/x86_64-linux-gnu -lpq

CFLAGS_DEBUG = -g -Og
# consider: -lto
CFLAGS_RELEASE = -O2 -DNDEBUG

CONFIG ?= debug
ifeq ($(CONFIG), debug)
    CFLAGS += $(CFLAGS_DEBUG)
else
    CFLAGS += $(CFLAGS_RELEASE)
endif

# Helpers 
rwildcard = $(foreach d,$(wildcard $(1)/*),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Files and directories
src_common := $(call rwildcard,src/common,*.c)
src_client := $(call rwildcard,src/client,*.c)
src_server := $(call rwildcard,src/server,*.c)
src_test := $(call rwildcard,test,*.c)
src_lib := $(call rwildcard,lib,*.c)

sql_dir := src/server/sql
src_sql := $(addprefix $(sql_dir)/, schema.sql tables.sql functions.sql views.sql triggers/*.sql data.sql)
src_sql_test := $(addprefix $(sql_dir)/, schema.sql tables.sql functions.sql views.sql triggers/*.sql test_data.sql)

bin_dir := bin
bin_client := $(bin_dir)/tchatator
bin_server := $(bin_dir)/tchatator-server
bin_test := $(bin_dir)/test

# Targets 

.PHONY: all client server test testdb db clean

# to call docker-gcc (currently unused)
# ./docker-gcc -o $@ -c '$(CFLAGS)' -l '$(LFLAGS)' $^

all: $(bin_client) $(bin_server)

clean:
	rm $(bin_dir)/*

client: src/client.c $(src_client) $(src_common) $(src_lib)
	mkdir -p $(bin_dir)
	$(CC) $(CFLAGS) -o $(bin_client) $^ $(LFLAGS_CLIENT)

server: src/server.c $(src_server) $(src_common) $(src_lib)
	mkdir -p $(bin_dir)
	$(CC) $(CFLAGS) -o $(bin_server) $^ $(LFLAGS_SERVER)

__DIR__ := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
test: $(bin_test)
	set -a; . $(__DIR__)/test.env; bin/test


$(bin_test): $(src_test) $(src_server) $(src_common) $(src_lib)
	mkdir -p $(bin_dir)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS_SERVER) -lm

testdb: SHELL:=/bin/bash
testdb: $(src_sql_test)
	@set -ea; . test.env; set +a; \
	echo "Creating database: $$DB_NAME"; \
	PGPASSWORD="$$DB_PASSWORD" createdb -U "$$DB_USER" -h "$$DB_HOST" -p "$$DB_PORT" "$$DB_NAME"; \
	echo "Preparing SQL file list..."; \
	sql_args=(); \
	for file in $^; do \
		echo "  < $$file"; \
		sql_args+=("-f" "$$file"); \
	done; \
	echo "Executing psql..."; \
	PGPASSWORD="$$DB_PASSWORD" psql -U "$$DB_USER" -h "$$DB_HOST" -p "$$DB_PORT" -d "$$DB_NAME" -v ON_ERROR_STOP=on "$${sql_args[@]}" -c 'commit;'

db: SHELL:=/bin/bash
db: $(src_sql)
	@set -ea; . .env; set +a; \
	echo "Creating database: $$DB_NAME"; \
	PGPASSWORD="$$DB_PASSWORD" createdb -U "$$DB_USER" -h "$$DB_HOST" -p "$$DB_PORT" "$$DB_NAME"; \
	echo "Preparing SQL file list..."; \
	sql_args=(); \
	for file in $^; do \
		echo "  < $$file"; \
		sql_args+=("-f" "$$file"); \
	done; \
	echo "Executing psql..."; \
	PGPASSWORD="$$DB_PASSWORD" psql -U "$$DB_USER" -h "$$DB_HOST" -p "$$DB_PORT" -d "$$DB_NAME" "$${sql_args[@]}" -c 'commit;'
