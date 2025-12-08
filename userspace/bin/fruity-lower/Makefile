# Makefile for fruity-lower - MSIL to Fruity IR Lowering Tool

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c99
# Include current directory FIRST to use stub headers instead of kernel headers
CFLAGS += -I.

# Source files
SOURCES = \
	msil_opcodes.c \
	msil_reader.c \
	fruity_lowering.c \
	stubs.c \
	main.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = fruity-lower

# Fruity IR sources (compiled directly without kernel lib)
FRUITY_SOURCES = ../../../kernel/clr/fruity/fruity_ir.c
FRUITY_OBJECTS = fruity_ir_userspace.o

all: $(TARGET)

# Compile Fruity IR for userspace (define types before including)
fruity_ir_userspace.o: $(FRUITY_SOURCES)
	$(CC) $(CFLAGS) -I. -c $(FRUITY_SOURCES) -o $@

$(TARGET): $(OBJECTS) $(FRUITY_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(FRUITY_OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
