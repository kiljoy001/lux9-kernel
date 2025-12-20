# Simple initrd Makefile

# Fixed secret key for development (seed)
# Public Key: 3b6a27bcceb6a42d62a3a8d02a6f0d73653215771de243a63ac048a18b59da29
SECRET_KEY=0000000000000000000000000000000000000000000000000000000000000000

all: ../initrd.tar

../host_sign: ../../host_sign.c ../../monocypher.c ../../monocypher.h
	gcc -o ../host_sign ../../host_sign.c -I../../

../initrd.tar: bin/* ../host_sign
	# Sign all files in bin/
	for f in bin/*; do \
		../host_sign $(SECRET_KEY) $$f; \
	done
	tar cf ../initrd.tar -C . .

clean:
	rm -f ../initrd.tar ../host_sign bin/*.sig
