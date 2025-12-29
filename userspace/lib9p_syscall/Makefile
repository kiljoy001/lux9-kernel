# Userspace init using shared kernel libc9
# Ensures 9P message format stays in sync

KERNEL = ../../kernel

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

# Use kernel's libc9 for 9P messages - single source of truth
KERNEL_LIBC9 = $(KERNEL)/libc9/convM2S.c $(KERNEL)/libc9/convS2M.c

OBJS = start.o init.o

all: init

init: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -T linker.ld $(OBJS) -o init

start.o: start.S
	$(AS) -o $@ $<

init.o: init.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o init

.PHONY: all clean
