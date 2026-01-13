# Makefile for Extensive WASM Test

KERNEL = ../../kernel
CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

all: wasm_test extensive_test.wasm

wasm_test: main.c exit.c
	$(CC) $(CFLAGS) -c -o main.o main.c
	$(CC) $(CFLAGS) -c -o exit.o exit.c
	$(AS) -o start.o ../lib9p_syscall/start.S
	$(LD) $(LDFLAGS) -T linker.ld start.o exit.o main.o -o wasm_test

extensive_test.wasm: extensive_test.wat
	wat2wasm extensive_test.wat -o extensive_test.wasm

clean:
	rm -f *.o wasm_test extensive_test.wasm
