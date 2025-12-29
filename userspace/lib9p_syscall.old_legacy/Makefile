# Makefile for pure 9P init using lib9p_syscall
#
# This builds a minimal ELF binary that uses only 9P message passing

KERNEL = ../../kernel

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -Iinclude -I$(KERNEL)/include -include u.h -include libc.h

ASFLAGS = 
LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

OBJS = start.o lib9p.o libexchange.o init.o convM2S.o convS2M.o

all: init

init: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -T linker.ld $(OBJS) -o init

start.o: start.S
	$(AS) $(ASFLAGS) -o $@ $<

# Rules to build kernel sources (convM2S.c, convS2M.c)
convM2S.o: $(KERNEL)/libc9/convM2S.c
	$(CC) $(CFLAGS) -c -o $@ $<

convS2M.o: $(KERNEL)/libc9/convS2M.c
	$(CC) $(CFLAGS) -c -o $@ $<

lib9p.o: lib9p.c
	$(CC) $(CFLAGS) -c -o $@ $<

libexchange.o: libexchange.c
	$(CC) $(CFLAGS) -c -o $@ $<

init.o: init.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o init

install: init
	@echo "Copying init to /boot/init in initrd..."
	cp init ../build/boot/init 2>/dev/null || \
	cp init ../../initrd/boot/init 2>/dev/null || \
	echo "Please manually copy init to initrd/boot/init"
