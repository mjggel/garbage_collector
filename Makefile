CC = gcc
CFLAGS = -Wall -Wextra -g3 -O0 -fno-stack-protector -Iinclude

BUILD_DIR = build
BIN_GC = $(BUILD_DIR)/gc_project
BIN_TEST = $(BUILD_DIR)/test_suite

all: $(BIN_GC) $(BIN_TEST)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_GC): src/main.c src/arena.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) src/main.c src/arena.c -o $(BIN_GC)

$(BIN_TEST): tests/stress_test.c src/arena.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/stress_test.c src/arena.c -o $(BIN_TEST)

clean:
	rm -rf $(BUILD_DIR)
