# Makefile for CIL Interpreter

# Compiler and flags
CC = gcc
CFLAGS = -I. -Wall -Wextra -std=c99 -g

# Directories
SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
FRAMEWORK_DIR = $(TEST_DIR)/framework
UNIT_TEST_DIR = $(TEST_DIR)/unit

# Source files
PE_PARSER_SRC = $(SRC_DIR)/pe_parser.c
CLI_PARSER_SRC = $(SRC_DIR)/cli_parser.c
IL_DECODER_SRC = $(SRC_DIR)/il_decoder.c
FRAMEWORK_SRC = $(FRAMEWORK_DIR)/test_runner.c

# Test files
PE_TEST_SRC = $(UNIT_TEST_DIR)/test_pe_parsing.c
CLI_TEST_SRC = $(UNIT_TEST_DIR)/test_cli_parsing.c
IL_TEST_SRC = $(UNIT_TEST_DIR)/test_il_decoding.c

# Targets
.PHONY: all clean test pe_test cli_test il_test

all: pe_test cli_test il_test

# PE parsing test
pe_test: $(PE_TEST_SRC) $(PE_PARSER_SRC) $(FRAMEWORK_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# CLI parsing test
cli_test: $(CLI_TEST_SRC) $(CLI_PARSER_SRC) $(PE_PARSER_SRC) $(FRAMEWORK_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# IL decoding test
il_test: $(IL_TEST_SRC) $(IL_DECODER_SRC) $(FRAMEWORK_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Run PE parsing tests
pe-test: pe_test
	./pe_test

# Run CLI parsing tests
cli-test: cli_test
	./cli_test

# Run IL decoding tests
il-test: il_test
	./il_test

# Run all tests
test: pe-test cli-test il-test

# Clean build artifacts
clean:
	rm -f pe_test cli_test il_test

# Install dependencies (if needed)
install-deps:
	@echo "No dependencies to install"