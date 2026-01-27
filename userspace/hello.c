#include <stddef.h>
typedef unsigned int uint32_t;
typedef int int32_t;

struct iovec {
    void  *buf;
    uint32_t buf_len;
};

// Import from WASI
__attribute__((import_module("wasi_snapshot_preview1"), import_name("fd_write")))
int32_t fd_write(int32_t fd, const struct iovec *iovs, uint32_t iovs_len, uint32_t *nwritten);

// Import from WASI
__attribute__((import_module("wasi_snapshot_preview1"), import_name("proc_exit")))
void proc_exit(int32_t rval);

void _start() {
    char *msg = "Hello from WASM Integration Test!\n";
    struct iovec iov;
    iov.buf = msg;
    iov.buf_len = 34; // Length of string
    uint32_t nwritten;
    fd_write(1, &iov, 1, &nwritten);
    proc_exit(0);
}
