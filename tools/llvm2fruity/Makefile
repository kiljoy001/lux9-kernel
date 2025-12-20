# llvm2fruity - LLVM to Fruity IR Compiler
#
# Bootstrap tool for compiling C++/SYCL to .NET DLLs.
# Run on development machine, not on Lux9 kernel.

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS =

SRCS = main.c llvm_parser.c llvm_to_fruity.c
OBJS = $(SRCS:.c=.o)
TARGET = llvm2fruity

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

# Dependencies
main.o: main.c llvm_parser.h
llvm_parser.o: llvm_parser.c llvm_parser.h
llvm_to_fruity.o: llvm_to_fruity.c llvm_parser.h
