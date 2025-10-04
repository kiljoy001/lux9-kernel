# Userspace programs Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include -I./lib/lib9p
LDFLAGS = -lext2fs -lcom_err

# Directories
SERVERDIR = servers
LIB9PDIR = lib/lib9p

# Lib9p sources
LIB9P_SRC = $(LIB9PDIR)/convM2S.c $(LIB9PDIR)/convS2M.c \
            $(LIB9PDIR)/convM2D.c $(LIB9PDIR)/convD2M.c \
            $(LIB9PDIR)/freefcall.c
LIB9P_OBJ = $(LIB9P_SRC:.c=.o)

# Servers
SERVERS = $(SERVERDIR)/ext4fs/ext4fs

# Utilities
BINDIR = bin
UTILS = $(BINDIR)/fscheck $(BINDIR)/init

all: $(SERVERS) $(UTILS)

# Build lib9p
$(LIB9PDIR)/%.o: $(LIB9PDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Build ext4fs server
$(SERVERDIR)/ext4fs/ext4fs: $(SERVERDIR)/ext4fs/ext4fs.o $(LIB9P_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SERVERDIR)/ext4fs/ext4fs.o: $(SERVERDIR)/ext4fs/ext4fs.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Build utilities
$(BINDIR)/fscheck: $(BINDIR)/fscheck.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BINDIR)/init: $(BINDIR)/init.c $(BINDIR)/syscalls.c
	$(CC) $(CFLAGS) -o $@ $^ -static -nostdlib -e main

# Initrd
INITRD_DIR = initrd
INITRD_TAR = initrd.tar

$(INITRD_TAR): $(SERVERS) $(UTILS)
	rm -rf $(INITRD_DIR)
	mkdir -p $(INITRD_DIR)/bin
	mkdir -p $(INITRD_DIR)/dev
	cp servers/ext4fs/ext4fs $(INITRD_DIR)/bin/
	cp bin/init $(INITRD_DIR)/bin/
	cp bin/fscheck $(INITRD_DIR)/bin/
	tar -cf $@ -C $(INITRD_DIR) .

initrd: $(INITRD_TAR)

clean:
	rm -f $(SERVERS) $(UTILS) $(SERVERDIR)/*/*.o $(LIB9PDIR)/*.o
	rm -rf $(INITRD_DIR) $(INITRD_TAR)

.PHONY: all clean initrd
