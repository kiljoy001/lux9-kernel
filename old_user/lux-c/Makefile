CC = gcc
AR = ar
GCC_INC := $(shell $(CC) -print-file-name=include)

CFLAGS = -nostdinc -fno-builtin -ffreestanding -m64 -O2 \
         -I./include \
         -I../../kernel/include \
         -I$(GCC_INC)

ASFLAGS = -m64

# Sources
SRCS = src/core.c src/memmove.c ../lib/lib9p/convS2M.c ../lib/lib9p/convM2S.c ../lib/lib9p/convD2M.c ../lib/lib9p/convM2D.c
OBJS = core.o memmove.o crt0.o convS2M.o convM2S.o convD2M.o convM2D.o

all: liblux-c.a

liblux-c.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)
	@echo "Built liblux-c.a"

crt0.o: src/crt0.S
	$(CC) $(ASFLAGS) -c $< -o $@

core.o: src/core.c
	$(CC) $(CFLAGS) -c $< -o $@

memmove.o: src/memmove.c
	$(CC) $(CFLAGS) -c $< -o $@

convS2M.o: ../lib/lib9p/convS2M.c
	$(CC) $(CFLAGS) -c $< -o $@

convM2S.o: ../lib/lib9p/convM2S.c
	$(CC) $(CFLAGS) -c $< -o $@

convD2M.o: ../lib/lib9p/convD2M.c
	$(CC) $(CFLAGS) -c $< -o $@

convM2D.o: ../lib/lib9p/convM2D.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o liblux-c.a

.PHONY: all clean
