#include <stddef.h>
#include <lux.h>

void main(void) {
    char *argv[] = {"/boot/hello.wasm", "arg1", "arg2", NULL};

    sys_print("exec_wasm: Attempting to exec /boot/hello.wasm...\n");

    sys_exec("/boot/hello.wasm", argv);

    // If exec returns, it means it failed
    sys_print("exec_wasm: Exec failed!\n");
    sys_exit("exec failed");
}

