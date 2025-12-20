# Simple initrd Makefile
all: ../initrd.tar

../initrd.tar: bin/*
	tar cf ../initrd.tar -C . .

clean:
	rm -f ../initrd.tar
