# Makefile for Lux9 Syscall Management System

# Directories
HDRDIR = .
SRCDIR = .

# Source files
CSRCS = \
    lux9_syscall_dispatch.c \
    lux9_syscall_table.c \
    lux9_syscall_compat.c \
    lux9_syscall_stats.c

# Header files
HDRS = \
    lux9_syscall.h \
    compat_plan9.h \
    compat_lux9_native.h \
    compat_rump.h \
    subsys_memory.h \
    internal.h

# Build targets
all: liblux9_syscall.a

# Compile with optimization and debugging info
CFLAGS = -O2 -g -Wall -Wextra -std=c11 -I$(HDRDIR)
CFLAGS += -DLUX9_SYSCALL_VERSION=\"1.0.0\"

# Create library
liblux9_syscall.a: $(CSRCS:.c=.o)
	ar rcs $@ $^

# Compile rules
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f *.o *.a

# Install headers
install-hdrs: $(HDRS)
	$(INSTALL) -d $(DESTDIR)/include/lux9/syscall
	$(INSTALL) -m 644 $(HDRS) $(DESTDIR)/include/lux9/syscall/

# Install library
install-lib: liblux9_syscall.a
	$(INSTALL) -d $(DESTDIR)/lib
	$(INSTALL) -m 644 liblux9_syscall.a $(DESTDIR)/lib/

# Complete install
install: install-hdrs install-lib

.PHONY: all clean install install-hdrs install-lib
