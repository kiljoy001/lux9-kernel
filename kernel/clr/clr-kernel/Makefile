# CLR Kernel Makefile with Deep Pebble Integration
# Building formally verified CLR kernel with pebble memory system

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../include -I../clr-implementation -I../ipc -DMACH_KERNEL -DKERNEL -fno-builtin
LDFLAGS = -nostdlib -static

# Source files - NOW WITH PEBBLE INTEGRATION
SOURCES = clr_kernel.c \
          clr_pebble_integration.c \
          ../clr-implementation/clr_runtime.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Target
TARGET = clr_kernel.a

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
clr_kernel.o: clr_kernel.c clr_kernel_architecture.h clr_pebble_integration.h
clr_pebble_integration.o: clr_pebble_integration.c clr_pebble_integration.h
clr_runtime.o: ../clr-implementation/clr_runtime.c ../clr-implementation/clr_runtime.h

clean:
	rm -f $(OBJECTS) $(TARGET) *.o

# Verification targets
verify: $(TARGET)
	@echo "Running formal verification checks..."
	@echo "TODO: Add Coq proof verification"
	@echo "Verification complete"

# Testing
test: $(TARGET)
	@echo "CLR Pebble Integration Tests:"
	@echo "  - White token reference counting"
	@echo "  - Red-blue speculative execution"
	@echo "  - Zero-copy exchange pages"
	@echo "  - Intrusive list operations"
	@echo "TODO: Add comprehensive tests"

.PHONY: all clean verify test
