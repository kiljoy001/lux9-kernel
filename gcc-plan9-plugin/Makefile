export GCC_PLUGIN_DIR := $(shell gcc -print-file-name=plugin)

CFLAGS := -I$(GCC_PLUGIN_DIR)/include -fPIC -shared
LDFLAGS :=

plan9.so: plan9.c
	@echo "GCC_PLUGIN_DIR=$(GCC_PLUGIN_DIR)"
	gcc -v $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f plan9.so
