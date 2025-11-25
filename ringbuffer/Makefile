CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c11 -pthread -Iinclude
LDFLAGS = -pthread

SOURCES = src/ringbuf.c src/security.c src/test_ringbuf.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = test_ringbuf

.PHONY: all clean test benchmark security parallel memory_test stats help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

%.o: src/%.c include/%.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	@echo "Running all tests..."
	./$(TARGET)

benchmark: $(TARGET)
	@echo "Running performance benchmark..."
	./$(TARGET) benchmark

security: $(TARGET)
	@echo "Running security tests..."
	./$(TARGET) security

parallel: $(TARGET)
	@echo "Running parallel processing test..."
	./$(TARGET) parallel

memory_test: $(TARGET)
	@echo "Running memory safety test with valgrind..."
	@echo "Install valgrind if not available: sudo apt install valgrind"
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./$(TARGET) memory_test

stats: $(TARGET)
	@echo "Showing ring buffer statistics..."
	./$(TARGET) stats

help:
	@echo "Lux9 Ring Buffer Security Test Suite"
	@echo "=================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make all         - Build the test program"
	@echo "  make clean       - Clean build artifacts"
	@echo "  make test        - Run all tests"
	@echo "  make benchmark   - Run performance benchmark"
	@echo "  make security    - Run security violation tests"
	@echo "  make parallel    - Run parallel processing test"
	@echo "  make memory_test - Run memory safety test with valgrind"
	@echo "  make stats       - Show ring buffer statistics"
	@echo "  make help        - Show this help message"
	@echo ""
	@echo "Direct usage:"
	@echo "  ./test_ringbuf           - Run all tests"
	@echo "  ./test_ringbuf benchmark - Performance benchmark only"
	@echo "  ./test_ringbuf security  - Security tests only"
	@echo "  ./test_ringbuf parallel  - Parallel processing test only"
	@echo "  ./test_ringbuf memory_test - Memory safety test only"
	@echo "  ./test_ringbuf stats     - Statistics only"
	@echo ""
	@echo "Dependencies:"
	@echo "  - gcc (C compiler)"
	@echo "  - pthread support"
	@echo "  - valgrind (for memory_test target)"

# Build info
build_info:
	@echo "Build Information:"
	@echo "  Compiler: $(CC)"
	@echo "  Flags: $(CFLAGS)"
	@echo "  Linker: $(LDFLAGS)"
	@echo "  Sources: $(SOURCES)"
	@echo "  Target: $(TARGET)"