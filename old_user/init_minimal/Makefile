all: init

init: init.o
	ld -nostdlib -static -Ttext=0x200000 -o init init.o

init.o: init.S
	as -o init.o init.S

clean:
	rm -f init init.o
