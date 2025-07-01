# Toolchain
CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -pthread -O2

# Directory layout
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

# Main binary (lab2)
BIN := $(BIN_DIR)/lab2

# Gather all .c under SRC_DIR
SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# --------------------------------------------------
# Target principale: build / run / clean
# --------------------------------------------------

.PHONY: build run clean
build: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

run: build
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) logs/*.log tests/_*.txt

# --------------------------------------------------
# Sub-system: POSIX MQ client & server
# --------------------------------------------------

BIN_CLIENT := $(BIN_DIR)/client
BIN_SERVER := $(BIN_DIR)/server

# Client: sends emergency_request_t
$(BIN_CLIENT): src/client.c include/models.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -Iinclude $< -o $@

# Server: parsing env.conf, listener, logging, queue, scheduler
$(BIN_SERVER): \
	src/server.c \
	src/mq_manager.c \
	src/parse_env.c \
	src/log.c \
       src/parse_rescuers.c \
       src/parse_emergency_types.c \
        src/digital_twin.c \
        src/queue.c \
        src/scheduler.c \
	include/models.h \
	include/mq_manager.h \
	include/parse_env.h \
	include/log.h \
        include/parse_rescuers.h \
        include/parse_emergency_types.h \
        include/digital_twin.h \
        include/queue.h \
        include/scheduler.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -Iinclude $(filter %.c,$^) -o $@

.PHONY: client server
client: $(BIN_CLIENT)
server: $(BIN_SERVER)

# --------------------------------------------------
# Unit tests
# --------------------------------------------------

# Test utils module
BIN_TEST_UTILS := $(BIN_DIR)/test_utils
$(BIN_TEST_UTILS): tests/test_utils.c include/utils.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -Iinclude $< -o $@

.PHONY: test-utils
test-utils: $(BIN_TEST_UTILS)
	./$(BIN_TEST_UTILS)

# Test parsers modules
BIN_TEST_PARSERS := $(BIN_DIR)/test_parsers
$(BIN_TEST_PARSERS): tests/test_parsers.c \
        src/parse_rescuers.c include/parse_rescuers.h \
        src/parse_emergency_types.c include/parse_emergency_types.h \
        src/log.c include/log.h
	@mkdir -p $(BIN_DIR)
		$(CC) $(CFLAGS) -Iinclude $^ -o $@
		chmod +x $@

.PHONY: test-parsers
test-parsers: $(BIN_TEST_PARSERS)
	./$(BIN_TEST_PARSERS)

