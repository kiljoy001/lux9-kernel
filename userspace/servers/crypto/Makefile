TARGET := cryptosrv
SRC := main.c
OBJ := $(SRC:.c=.o)

# Get GCC internal headers
GCC_INC := $(shell $(CC) -print-file-name=include)

CFLAGS := -nostdinc \
          -I./include \
          -I../../include \
          -I../../../kernel/include \
          -I$(GCC_INC) \
          -Wall -Wextra

# Link against crt0.o (startup) and libc.a
LDFLAGS := -nostdlib ../../lib/crt0.o -L../../lib -lc

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)