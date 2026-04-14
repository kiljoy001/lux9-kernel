# Procd - Userspace Process Manager

CC = gcc
CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fno-pic -fno-pie -O2 -Wall \
         -fcf-protection=none \
         -I../../kernel/include \
         -I../lib/liblux/inc

LIBLUX = ../lib/liblux
INIT_DIR = ../lib9p_syscall

.PHONY: all clean

all: procd

procd: procd.c $(LIBLUX)/liblux.a $(INIT_DIR)/start.o $(INIT_DIR)/exit.o
	$(CC) $(CFLAGS) -no-pie -o $@ procd.c $(INIT_DIR)/start.o $(INIT_DIR)/exit.o -L$(LIBLUX) -llux

clean:
	rm -f procd *.o
