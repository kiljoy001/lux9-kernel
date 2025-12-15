# GHOSTDAG Standalone Test

CC = gcc
CFLAGS = -Wall -Wextra -g -O2

all: test_ghostdag

test_ghostdag: test_ghostdag.c ghostdag.c ghostdag.h
	$(CC) $(CFLAGS) -o test_ghostdag test_ghostdag.c ghostdag.c

clean:
	rm -f test_ghostdag *.o

test: test_ghostdag
	./test_ghostdag

# Test with different K parameters
test-k1: test_ghostdag
	./test_ghostdag 1

test-k5: test_ghostdag
	./test_ghostdag 5

.PHONY: all clean test test-k1 test-k5
