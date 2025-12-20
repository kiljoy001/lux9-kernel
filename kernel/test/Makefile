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
	rm -f $(TEST_BIN)

.PHONY: all run clean
