SHELL := /bin/bash

CC ?= gcc
CSTD ?= c11
MODE ?= release
TARGET ?= billing_system
BUILD_DIR ?= build
FRONTEND_DIR ?= frontend
LAB ?= exp3
TEST_SUITE ?= menu

SRC_DIR := src
INCLUDE_DIR := include
OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/dep
BIN_DIR := $(BUILD_DIR)/bin
BIN := $(BIN_DIR)/$(TARGET)
TEST_OUTPUT := $(BUILD_DIR)/test_output.txt
MODE_STAMP := $(BUILD_DIR)/.mode_stamp
TEST_DIR ?= tests/$(LAB)
TEST_INPUT ?= $(TEST_DIR)/$(TEST_SUITE).input
TEST_EXPECT ?= $(TEST_DIR)/$(TEST_SUITE).expect.tsv
TEST_RUNNER ?= scripts/run_smoke_test.sh
TEST_BATCH_RUNNER ?= scripts/run_test_batch.sh
TEST_GROUP ?= acceptance
TEST_SUITES_FILE ?= $(TEST_DIR)/suites.$(TEST_GROUP)
TEST_OUTPUT_DIR ?= $(BUILD_DIR)

SRC := $(shell find $(SRC_DIR) -type f -name '*.c' | sort)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEP := $(patsubst $(SRC_DIR)/%.c,$(DEP_DIR)/%.d,$(SRC))

CPPFLAGS ?= -I$(INCLUDE_DIR)
CFLAGS_BASE := -std=$(CSTD) -Wall -Wextra -pedantic

ifeq ($(MODE),debug)
	CFLAGS_MODE := -O0 -g -DDEBUG
else ifeq ($(MODE),release)
	CFLAGS_MODE := -O2 -DNDEBUG
else
$(error Invalid MODE='$(MODE)'. Use MODE=debug or MODE=release)
endif

CFLAGS := $(CFLAGS_BASE) $(CFLAGS_MODE)

FRONTEND_PM := $(shell if command -v pnpm >/dev/null 2>&1; then echo pnpm; elif command -v npm >/dev/null 2>&1; then echo npm; elif command -v yarn >/dev/null 2>&1; then echo yarn; fi)

.PHONY: all build debug release run test test-batch clean distclean rebuild FORCE \
	frontend-install frontend-build frontend-dev help

all: build

build: $(BIN)

release:
	@$(MAKE) MODE=release build

debug:
	@$(MAKE) MODE=debug build

run: build
	@$(BIN)

test: build
	@bash "$(TEST_RUNNER)" "$(BIN)" "$(TEST_INPUT)" "$(TEST_EXPECT)" "$(TEST_OUTPUT)"

test-batch: build
	@bash "$(TEST_BATCH_RUNNER)" "$(BIN)" "$(TEST_DIR)" "$(TEST_SUITES_FILE)" "$(TEST_OUTPUT_DIR)" "$(TEST_RUNNER)"

rebuild: clean build

clean:
	@rm -rf $(BUILD_DIR)

distclean: clean
	@if [ -d "$(FRONTEND_DIR)/node_modules" ]; then \
		rm -rf "$(FRONTEND_DIR)/node_modules"; \
		echo "[distclean] removed $(FRONTEND_DIR)/node_modules"; \
	else \
		echo "[distclean] skip: $(FRONTEND_DIR)/node_modules not found"; \
	fi

frontend-install:
	@if [ ! -d "$(FRONTEND_DIR)" ]; then \
		echo "[frontend] skip: '$(FRONTEND_DIR)' directory not found"; \
		exit 0; \
	fi; \
	pm="$(FRONTEND_PM)"; \
	if [ -z "$$pm" ]; then \
		echo "[frontend] error: no package manager found (pnpm/npm/yarn)"; \
		exit 1; \
	fi; \
	echo "[frontend] using $$pm install in $(FRONTEND_DIR)"; \
	if [ "$$pm" = "yarn" ]; then \
		(cd "$(FRONTEND_DIR)" && yarn install); \
	else \
		(cd "$(FRONTEND_DIR)" && $$pm install); \
	fi

frontend-build:
	@if [ ! -d "$(FRONTEND_DIR)" ]; then \
		echo "[frontend] skip: '$(FRONTEND_DIR)' directory not found"; \
		exit 0; \
	fi; \
	pm="$(FRONTEND_PM)"; \
	if [ -z "$$pm" ]; then \
		echo "[frontend] error: no package manager found (pnpm/npm/yarn)"; \
		exit 1; \
	fi; \
	echo "[frontend] using $$pm build in $(FRONTEND_DIR)"; \
	if [ "$$pm" = "yarn" ]; then \
		(cd "$(FRONTEND_DIR)" && yarn build); \
	elif [ "$$pm" = "npm" ]; then \
		(cd "$(FRONTEND_DIR)" && npm run build); \
	else \
		(cd "$(FRONTEND_DIR)" && pnpm build); \
	fi

frontend-dev:
	@if [ ! -d "$(FRONTEND_DIR)" ]; then \
		echo "[frontend] skip: '$(FRONTEND_DIR)' directory not found"; \
		exit 0; \
	fi; \
	pm="$(FRONTEND_PM)"; \
	if [ -z "$$pm" ]; then \
		echo "[frontend] error: no package manager found (pnpm/npm/yarn)"; \
		exit 1; \
	fi; \
	echo "[frontend] using $$pm dev in $(FRONTEND_DIR)"; \
	if [ "$$pm" = "yarn" ]; then \
		(cd "$(FRONTEND_DIR)" && yarn dev); \
	elif [ "$$pm" = "npm" ]; then \
		(cd "$(FRONTEND_DIR)" && npm run dev); \
	else \
		(cd "$(FRONTEND_DIR)" && pnpm dev); \
	fi

help:
	@echo "Usage:"
	@echo "  make [MODE=release|debug]         Build C program (default MODE=release)"
	@echo "  make build                        Build target binary"
	@echo "  make debug                        Build with debug flags"
	@echo "  make release                      Build with release flags"
	@echo "  make run                          Build and run binary"
	@echo "  make test                         Run test suite from files via $(TEST_RUNNER)"
	@echo "  make test-batch                   Run suites listed in TEST_SUITES_FILE"
	@echo "  make clean                        Remove $(BUILD_DIR)"
	@echo "  make distclean                    clean + remove frontend node_modules"
	@echo "  make frontend-install             Install frontend dependencies"
	@echo "  make frontend-build               Build frontend app"
	@echo "  make frontend-dev                 Run frontend dev server"
	@echo "Variables:"
	@echo "  TARGET=$(TARGET) BUILD_DIR=$(BUILD_DIR) FRONTEND_DIR=$(FRONTEND_DIR)"
	@echo "  MODE=$(MODE) LAB=$(LAB) TEST_SUITE=$(TEST_SUITE)"
	@echo "  TEST_GROUP=$(TEST_GROUP) TEST_SUITES_FILE=$(TEST_SUITES_FILE)"
	@echo "  TEST_INPUT=$(TEST_INPUT)"
	@echo "  TEST_EXPECT=$(TEST_EXPECT)"

$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(MODE_STAMP)
	@mkdir -p "$(dir $@)" "$(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@)" -c $< -o $@

$(BIN_DIR):
	@mkdir -p $@

$(MODE_STAMP): FORCE
	@tmp_file="$@.tmp"; \
	mkdir -p "$(BUILD_DIR)"; \
	printf 'MODE=%s\nCFLAGS=%s\n' "$(MODE)" "$(CFLAGS)" > "$$tmp_file"; \
	if [ ! -f "$@" ] || ! cmp -s "$$tmp_file" "$@"; then \
		mv "$$tmp_file" "$@"; \
	else \
		rm -f "$$tmp_file"; \
	fi

FORCE:

-include $(DEP)
