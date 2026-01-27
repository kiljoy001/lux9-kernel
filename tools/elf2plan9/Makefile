# elf2plan9 Makefile

CC = gcc
CFLAGS = -Wall -O2

elf2plan9: elf2plan9.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f elf2plan9

.PHONY: clean
