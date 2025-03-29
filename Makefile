# === Config ===
CC := gcc
# todo: readd -pedantic when we get C23 support
# -Wmissing-prototypes
CFLAGS := -std=gnu2x -Wall -Wextra \
         -Wcast-qual -Wcast-align -Wstrict-aliasing -Wpointer-arith \
         -Winit-self -Wshadow -Wstrict-prototypes -Wformat -Wno-format-zero-length \
         -Wredundant-decls -Wfloat-equal -Wundef -Wvla -Wno-parentheses \
         -Ilib/own -isystem lib/vendor \
		 -D__SKIP_GNU

# CLFAGS += -fanalyzer # takes time
# CLFAGS += -fsanitize=address,leak,undefined # messes with debugging

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

# === Helpers ===
rwildcard = $(foreach d,$(wildcard $(1)/*),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC_COMMON := $(call rwildcard,src/common,*.c)
SRC_CLIENT := $(call rwildcard,src/client,*.c)
SRC_SERVER := $(call rwildcard,src/server,*.c)
SRC_TEST := $(call rwildcard,test,*.c)
SRC_LIB := $(call rwildcard,lib,*.c)

BIN_DIR := bin
BIN_CLIENT := $(BIN_DIR)/tchatator
BIN_SERVER := $(BIN_DIR)/tchatator-server
BIN_TEST := $(BIN_DIR)/test

# === Targets ===
.PHONY: all client server clean

# to call docker-gcc (currently unused)
# ./docker-gcc -o $@ -c '$(CFLAGS)' -l '$(LFLAGS)' $^

all: $(BIN_CLIENT) $(BIN_SERVER)

clean:
	rm $(BIN_DIR)/*

$(BIN_CLIENT): src/client.c $(SRC_CLIENT) $(SRC_COMMON) $(SRC_LIB)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS_CLIENT)

$(BIN_SERVER): src/server.c $(SRC_SERVER) $(SRC_COMMON) $(SRC_LIB)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS_SERVER)

$(BIN_TEST): $(SRC_TEST) $(SRC_SERVER) $(SRC_COMMON) $(SRC_LIB)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS_SERVER) -lm
