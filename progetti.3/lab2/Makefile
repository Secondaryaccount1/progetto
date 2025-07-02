# Toolchain
CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -pthread -O2

# Directory layout
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

# No monolithic binary is produced. Server and client are built separately.


# --------------------------------------------------
# Target principale: build / run / clean
# --------------------------------------------------

.PHONY: build run clean
build: server

run: server
	./$(BIN_SERVER)

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

# Server: listener and scheduler (use -r/-e/-n to override conf paths)
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
	$(CC) $(CFLAGS) -Iinclude $(filter %.c,$^) -o $@ -lrt

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

# Test parse_env module
BIN_TEST_PARSE_ENV := $(BIN_DIR)/test_parse_env
$(BIN_TEST_PARSE_ENV): tests/test_parse_env.c src/parse_env.c include/parse_env.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -Iinclude $^ -o $@

.PHONY: test-parse-env
test-parse-env: $(BIN_TEST_PARSE_ENV)
	./$(BIN_TEST_PARSE_ENV)

# Deadlock monitor test
BIN_TEST_DEADLOCK := $(BIN_DIR)/test_deadlock
$(BIN_TEST_DEADLOCK): tests/test_deadlock.c src/scheduler.c src/log.c include/scheduler.h include/log.h include/models.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -Iinclude $^ -o $@

.PHONY: test-deadlock
test-deadlock: $(BIN_TEST_DEADLOCK)
	./$(BIN_TEST_DEADLOCK)

# Scheduler integration test
BIN_TEST_SCHEDULER := $(BIN_DIR)/test_scheduler
$(BIN_TEST_SCHEDULER): tests/test_scheduler.c \
    src/scheduler.c src/queue.c src/log.c \
    include/scheduler.h include/queue.h include/log.h include/models.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -Iinclude $^ -o $@

.PHONY: test-scheduler
test-scheduler: $(BIN_TEST_SCHEDULER)
	./$(BIN_TEST_SCHEDULER)
