# izi

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -Iinclude
LDFLAGS = -lm

SRC_DIR = src
STDLIB_DIR = stdlib
KERNEL_DIR = kernel
BUILD_DIR = build


COMPILER_SRCS = $(filter-out $(SRC_DIR)/parser_legacy.c, $(wildcard $(SRC_DIR)/*.c))
COMPILER_OBJS = $(COMPILER_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
COMPILER_BIN  = $(BUILD_DIR)/uwucc

STDLIB_SRCS = $(STDLIB_DIR)/uwu_stdlib.c
STDLIB_OBJ  = $(BUILD_DIR)/uwu_stdlib.o

.PHONY: all compiler stdlib kernel clean help

all: compiler stdlib

help:
	@echo "Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make compiler    - Build the UwU-C compiler"
	@echo "  make stdlib      - Build the standard library"
	@echo "  make kernel      - Build the UwUOS kernel"
	@echo "  make all         - Build compiler and stdlib (default)"
	@echo "  make clean       - Clean all build artifacts"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---------- compiler ----------

compiler: $(BUILD_DIR) $(COMPILER_BIN)

$(COMPILER_BIN): $(COMPILER_OBJS)
	$(CC) $(COMPILER_OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------- stdlib ----------

stdlib: $(BUILD_DIR) $(STDLIB_OBJ)

$(STDLIB_OBJ): $(STDLIB_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------- kernel ----------

kernel:
	$(MAKE) -C $(KERNEL_DIR)

# ---------- clean ----------

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(KERNEL_DIR) clean
