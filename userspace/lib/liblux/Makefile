# LibLux Makefile

# Compiler settings
CC=gcc
KERNEL=../../../kernel
CFLAGS=-Iinc -I$(KERNEL)/include -D_LOCK_H_ -Wall -Wextra -nostdlib \
       -ffreestanding -fno-builtin -fno-stack-protector -mno-red-zone \
       -mcmodel=small -fno-pic -fno-pie -fcf-protection=none
AR=ar

# Source files
SRC = src/syscalls.c \
      src/exchange.c \
      src/string.c \
      src/pebble.c \
      src/msgord.c \
      src/convM2S.c \
      src/convS2M.c \
      src/server9p.c \
      src/printf.c \
      src/convD2M.c \
      src/srv_ring.c \
      src/wasm_fileserver.c

# Object files
OBJ = $(SRC:.c=.o)

# Assembly files
ASM_SRC = src/syscall_asm.S
ASM_OBJ = $(ASM_SRC:.S=.o)

# Output library
TARGET = liblux.a

# Rules
all: $(TARGET)

$(TARGET): $(OBJ) $(ASM_OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# WASM fileserver standalone executable
wasm_fileserver: src/wasm_fileserver.o $(OBJ) $(ASM_OBJ)
	$(CC) -o $@ $^ -ldl -lpthread

clean:
	rm -f $(OBJ) $(ASM_OBJ) $(TARGET) wasm_fileserver

.PHONY: all clean
