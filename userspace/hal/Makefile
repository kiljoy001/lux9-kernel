# HAL - Hardware Abstraction Layer Server
# Manages device families and resource allocation

KERNEL = ../../kernel
LIBLUX = ../lib/liblux

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include \
         -I$(LIBLUX)/inc \
         -I.

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

OBJS = start.o exit.o hal.o family_registry.o channel_manager.o resource_pool.o \
       family_pci.o family_tpm.o transaction.o rump_integration.o

EXIT_OBJ = ../lib9p_syscall/exit.o

all: $(EXIT_OBJ) hal

hal: $(OBJS) $(LIBLUX)/liblux.a linker.ld
	$(LD) $(LDFLAGS) -T linker.ld $(OBJS) $(LIBLUX)/liblux.a -o hal

start.o: start.S
	$(AS) -o $@ $<

$(EXIT_OBJ):
	$(MAKE) -C ../lib9p_syscall exit.o

exit.o: $(EXIT_OBJ)
	cp $(EXIT_OBJ) exit.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o hal

.PHONY: all clean
