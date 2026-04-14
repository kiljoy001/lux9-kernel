# Symbolicd - Userspace Symbolic Reasoning Server

CC = gcc
CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fno-pic -fno-pie -O2 -Wall \
         -fcf-protection=none \
         -I../../kernel/include \
         -I../lib/liblux/inc \
         -DMINI_GMP_KERNEL

LIBLUX = ../lib/liblux
INIT_DIR = ../lib9p_syscall

.PHONY: all clean

all: symbolicd

symbolicd: symbolicd.c mini-gmp.h $(LIBLUX)/liblux.a $(INIT_DIR)/start.o $(INIT_DIR)/exit.o
	$(CC) $(CFLAGS) -no-pie -o $@ symbolicd.c $(INIT_DIR)/start.o $(INIT_DIR)/exit.o -L$(LIBLUX) -llux

clean:
	rm -f symbolicd *.o
