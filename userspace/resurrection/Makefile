# Makefile for Resurrection Server
# Layer 2 service monitor using 9P syscalls (no POSIX)

KERNEL = ../../kernel

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

TARGET = resurrection
SRC = resurrection.c
OBJ = $(SRC:.c=.o)

# Use same start.S as init
START_OBJ = ../lib9p_syscall/start.o

all: $(START_OBJ) $(TARGET)

$(TARGET): $(START_OBJ) $(OBJ) ../lib9p_syscall/linker.ld
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(START_OBJ) $(OBJ) -o $@
	@echo "Built resurrection server: $@"
	@ls -lh $@

$(START_OBJ):
	$(MAKE) -C ../lib9p_syscall start.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)
	@echo "Cleaned resurrection build"

.PHONY: all clean
