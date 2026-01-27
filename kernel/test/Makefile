# CLR Unit Tests Makefile

CC = g++
CFLAGS = -Wall -Wno-write-strings -I.. -I../include -I../clr/fruity -std=c++11 -g

# Source files needed from CLR
CLR_SOURCES = \
	../clr/fruity/fruity_to_qbe.c \
	../clr/fruity/qbe_buffer.c \
	../clr/qbe_compile.c

# Test executable
TEST_BIN = test_clr_pipeline

all: $(TEST_BIN)

$(TEST_BIN): test_clr_pipeline.c $(CLR_SOURCES)
	@echo "Building CLR unit tests..."
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Build complete: $@"

run: $(TEST_BIN)
	@echo ""
	@echo "Running CLR unit tests..."
	@echo ""
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN) test_cap

# Capability System Tests (C only, no C++)
# Uses real Monocypher for proper cryptographic testing
CAP_CC = gcc
CAP_CFLAGS = -Wall -I.. -I../include -I../crypto -g -DTEST_STANDALONE \
             -Wno-discarded-qualifiers -Wno-unknown-pragmas

CAP_SOURCES = \
	test_blind_cap.c \
	test_cap_integration.c \
	test_cap_boot.c \
	test_cap_runner.c \
	test_stubs.c \
	../crypto/blind_cap.c \
	../crypto/monocypher.c

test_cap: $(CAP_SOURCES)
	@echo "Building capability tests with real Monocypher..."
	$(CAP_CC) $(CAP_CFLAGS) -o $@ test_cap_runner.c test_blind_cap.c \
		test_cap_integration.c test_cap_boot.c test_stubs.c \
		../crypto/blind_cap.c ../crypto/monocypher.c
	@echo "Build complete: $@"

run_cap: test_cap
	@echo ""
	@echo "Running capability tests..."
	@echo ""
	./test_cap

.PHONY: all run clean run_cap
