# Makefile for TurboCID Filesystem
# Userspace Semantic Filesystem

KERNEL = ../../kernel

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall \
         -fcf-protection=none \
         -I$(KERNEL)/include

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

TARGET = turbocid
SRC = turbocid.c
OBJ = $(SRC:.c=.o)

# Use same start.S as init/resurrection
START_OBJ = ../lib9p_syscall/start.o
EXIT_OBJ = ../lib9p_syscall/exit.o

all: $(START_OBJ) $(EXIT_OBJ) $(TARGET)

$(TARGET): $(START_OBJ) $(EXIT_OBJ) $(OBJ) ../lib9p_syscall/linker.ld
	$(LD) $(LDFLAGS) -T ../lib9p_syscall/linker.ld $(START_OBJ) $(EXIT_OBJ) $(OBJ) -o $@
	@echo "Built TurboCID: $@"
	@ls -lh $@

$(START_OBJ):
	$(MAKE) -C ../lib9p_syscall start.o

$(EXIT_OBJ):
	$(MAKE) -C ../lib9p_syscall exit.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
