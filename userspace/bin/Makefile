# Makefile for exchange test program

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Target
TARGET = test_exchange_9p

# Default target
all: $(TARGET)

# Compile the test program
$(TARGET): test_exchange_9p.c
	$(CC) $(CFLAGS) -o $@ $<

# Clean build files
clean:
	rm -f $(TARGET)

.PHONY: all clean