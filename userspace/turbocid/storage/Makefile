# Makefile for Sartfs Storage Engine
# Sophia Adaptive Radix Tree Filesystem - Block Storage Layer

KERNEL = ../../../kernel
LIBLUX = ../../lib/liblux
MONOCYPHER = ../../../monocypher-4.0.2/src

CC = gcc
LD = ld
AR = ar

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -mno-red-zone -mcmodel=small -fPIC -O2 -Wall -Wextra \
         -fcf-protection=none \
         -I$(KERNEL)/include -I$(LIBLUX)/inc -I$(MONOCYPHER) -I.

LDFLAGS = -nostdlib -static -no-pie --no-dynamic-linker -z max-page-size=0x1000

# Source files
SRCS = sart_journal.c sart_store.c sart_art.c sart_blkio.c
OBJS = $(SRCS:.c=.o)

# Delta compression sources (Phase 1)
DELTA_SRCS = delta/xdelta3_simple.c delta/content_detect.c delta/delta_encode.c
DELTA_OBJS = $(DELTA_SRCS:.c=.o)

# Semantic clustering sources (Phase 2)
SEMANTIC_SRCS = semantic/tlsh.c
SEMANTIC_OBJS = $(SEMANTIC_SRCS:.c=.o)

# All objects
ALL_OBJS = $(OBJS) $(DELTA_OBJS) $(SEMANTIC_OBJS)

# Monocypher object (we compile it locally)
MONOCYPHER_SRC = $(MONOCYPHER)/monocypher.c
MONOCYPHER_OBJ = monocypher.o

# Test sources
TEST_SRC = test_immutability.c
TEST_OBJ = $(TEST_SRC:.c=.o)

# Output library
LIB = libsart.a

# Test binary
TEST_BIN = test_immutability

# Start object and linker script (from lib9p_syscall)
START_OBJ = ../../lib9p_syscall/start.o
LINKER_SCRIPT = ../../lib9p_syscall/linker.ld

# liblux library
LIBLUX_A = $(LIBLUX)/liblux.a

.PHONY: all clean lib test

all: lib

lib: $(LIB)

$(LIB): $(ALL_OBJS) $(MONOCYPHER_OBJ)
	$(AR) rcs $@ $(ALL_OBJS) $(MONOCYPHER_OBJ)
	@echo "Built Sartfs library: $@"

# Monocypher compilation (needs -Wno-unused-parameter for clean build)
$(MONOCYPHER_OBJ): $(MONOCYPHER_SRC)
	$(CC) $(CFLAGS) -Wno-unused-parameter -c -o $@ $<

test: $(TEST_BIN)
	@echo "Test binary built: $@"
	@echo "Run on Lux9 to execute tests"

$(TEST_BIN): $(START_OBJ) $(TEST_OBJ) $(OBJS) $(MONOCYPHER_OBJ) $(LIBLUX_A)
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) \
		$(START_OBJ) $(TEST_OBJ) $(OBJS) $(MONOCYPHER_OBJ) $(LIBLUX_A) -o $@
	@echo "Built test binary: $@"
	@ls -lh $@

$(START_OBJ):
	$(MAKE) -C ../../lib9p_syscall start.o

$(LIBLUX_A):
	$(MAKE) -C $(LIBLUX)

%.o: %.c dat.h hjfs_dat.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(ALL_OBJS) $(TEST_OBJ) $(MONOCYPHER_OBJ) $(LIB) $(TEST_BIN)

# Dependencies
sart_journal.o: sart_journal.c dat.h hjfs_dat.h
sart_store.o: sart_store.c dat.h hjfs_dat.h
sart_art.o: sart_art.c dat.h hjfs_dat.h
test_immutability.o: test_immutability.c dat.h
