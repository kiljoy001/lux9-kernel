# Makefile for TurboCID Filesystem
# Userspace Semantic Filesystem

KERNEL = ../../kernel

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include \
         -I../lib/liblux/inc

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

TARGET = turbocid
SOPHIA = sophia
SRC = turbocid.c
SOPHIA_SRC = sophia.c
OBJ = $(SRC:.c=.o)
SOPHIA_OBJ = $(SOPHIA_SRC:.c=.o)

# Use same start.S as init/resurrection
START_OBJ = ../lib9p_syscall/start.o
EXIT_OBJ = ../lib9p_syscall/exit.o

all: $(START_OBJ) $(EXIT_OBJ) $(TARGET) $(SOPHIA)

$(TARGET): $(START_OBJ) $(EXIT_OBJ) $(OBJ) ../lib9p_syscall/linker.ld
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(START_OBJ) $(EXIT_OBJ) $(OBJ) -L../lib/liblux -llux -o $@
	@echo "Built TurboCID: $@"
	@ls -lh $@

$(SOPHIA): $(START_OBJ) $(EXIT_OBJ) $(SOPHIA_OBJ) ../lib9p_syscall/linker.ld storage/libsart.a
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(START_OBJ) $(EXIT_OBJ) $(SOPHIA_OBJ) -L../lib/liblux -llux -Lstorage -lsart -o $@
	@echo "Built Sophia: $@"
	@ls -lh $@

storage/libsart.a:
	$(MAKE) -C storage all

$(START_OBJ):
	$(MAKE) -C ../lib9p_syscall start.o

$(EXIT_OBJ):
	$(MAKE) -C ../lib9p_syscall exit.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ) $(SOPHIA) $(SOPHIA_OBJ)

.PHONY: all clean
