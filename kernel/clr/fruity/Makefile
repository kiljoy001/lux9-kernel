# Fruity IR Makefile
# Building the Fruity Pebbles Intermediate Representation

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../include -I../clr-implementation -I../clr-kernel -I../qbe -DMACH_KERNEL -DKERNEL -fno-builtin

# Source files
SOURCES = fruity_ir.c fruity_to_qbe.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Target
TARGET = fruity.a

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
fruity_ir.o: fruity_ir.c fruity_ir.h fruity_opcodes.h ../clr-kernel/clr_pebble_integration.h
fruity_to_qbe.o: fruity_to_qbe.c fruity_to_qbe.h fruity_ir.h fruity_opcodes.h ../qbe/exchange_io.h

clean:
	rm -f $(OBJECTS) $(TARGET) *.o

.PHONY: all clean
