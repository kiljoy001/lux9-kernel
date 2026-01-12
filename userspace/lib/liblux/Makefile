# LibLux Makefile

# Compiler settings
CC=gcc
KERNEL=../../../kernel
CFLAGS=-Iinc -I$(KERNEL)/include -D_LOCK_H_ -fPIC -Wall -Wextra -nostdlib -ffreestanding
AR=ar

# Source files
SRC = src/syscalls.c \
      src/string.c \
      src/pebble.c \
      src/msgord.c \
      src/convM2S.c \
      src/convS2M.c \
      src/server9p.c \
      src/printf.c

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

clean:
	rm -f $(OBJ) $(ASM_OBJ) $(TARGET)

.PHONY: all clean
