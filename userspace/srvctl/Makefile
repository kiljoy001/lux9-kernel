# srvctl - Service registration helper
# Registers plain executables with the resurrection server via /srv

KERNEL = ../../kernel
LIBLUX = ../lib/liblux

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include \
         -I$(LIBLUX)/inc

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

OBJS = start.o srvctl.o

all: srvctl

srvctl: $(OBJS) $(LIBLUX)/liblux.a linker.ld
	$(LD) $(LDFLAGS) -T linker.ld $(OBJS) $(LIBLUX)/liblux.a -o srvctl

start.o: start.S
	$(AS) -o $@ $<

srvctl.o: srvctl.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o srvctl

.PHONY: all clean
