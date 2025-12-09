CC = gcc
AR = ar
GCC_INC := $(shell $(CC) -print-file-name=include)

CFLAGS = -nostdlib -fno-builtin -ffreestanding -m64 -O2 \
         -I./include \
         -I../../kernel/include \
         -I$(GCC_INC)

ASFLAGS = -m64

# Sources
SRCS = src/core.c
OBJS = core.o crt0.o

all: liblux-c.a

liblux-c.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)
	@echo "Built liblux-c.a"

crt0.o: src/crt0.S
	$(CC) $(ASFLAGS) -c $< -o $@

core.o: src/core.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o liblux-c.a

.PHONY: all clean
