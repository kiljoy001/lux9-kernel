# Userspace init using liblux library
# Ensures 9P message format stays in sync

KERNEL = ../../kernel
LIBLUX = ../lib/liblux

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include \
         -I$(LIBLUX)/inc

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

OBJS = start.o init.o

all: init

init: $(OBJS) $(LIBLUX)/liblux.a linker.ld
	$(LD) $(LDFLAGS) -T linker.ld $(OBJS) $(LIBLUX)/liblux.a -o init

start.o: start.S
	$(AS) -o $@ $<

init.o: init.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o init

.PHONY: all clean
