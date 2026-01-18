# Makefile for HAL server

KERNEL = ../../kernel

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include \
         -I../lib/liblux/inc

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

TARGET = hal
SRC = hal.c
OBJ = $(SRC:.c=.o)

START_OBJ = ../lib9p_syscall/start.o
EXIT_OBJ = ../lib9p_syscall/exit.o

all: $(START_OBJ) $(EXIT_OBJ) $(TARGET)

$(TARGET): $(START_OBJ) $(EXIT_OBJ) $(OBJ) ../lib9p_syscall/linker.ld
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(START_OBJ) $(EXIT_OBJ) $(OBJ) -L../lib/liblux -llux -o $@
	@echo "Built HAL: $@"
	@ls -lh $@

$(START_OBJ):
	$(MAKE) -C ../lib9p_syscall start.o

$(EXIT_OBJ):
	$(MAKE) -C ../lib9p_syscall exit.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
