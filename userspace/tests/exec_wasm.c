#include <stddef.h>
#include <lux.h>

void main(void) {
    char *argv[] = {"/boot/wasm_smoke.wasm", "wasm_smoke", NULL};

    sys_print("exec_wasm: attempting to exec /boot/wasm_smoke.wasm...\n");

    sys_exec("/boot/wasm_smoke.wasm", argv);

    // If exec returns, it means it failed
    sys_print("exec_wasm: exec failed!\n");
    sys_exit("exec failed");
}
