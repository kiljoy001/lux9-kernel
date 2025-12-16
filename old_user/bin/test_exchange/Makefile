# Makefile for exchange test program

# Compiler and flags
CC = 9c
AS = 9a
LD = 9l

CFLAGS = -I../../include
ASFLAGS = 
LDFLAGS = -static

# Source files
SRCS = main.c
OBJS = $(SRCS:.c=.o)

# Target
TARGET = test_exchange

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# Assembly source files (if any)
%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)

# Install to userspace/bin
install: $(TARGET)
	cp $(TARGET) ../../bin/

.PHONY: all clean install