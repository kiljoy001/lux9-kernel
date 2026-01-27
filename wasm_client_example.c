/* wasm_client_example.c - Client Example for WASM Fileserver
 *
 * Demonstrates how clients interact with the simplified WASM fileserver
 * using standard 9P operations for compilation and execution.
 *
 * This shows the clean separation: 9P operations for file management,
 * WASM runtime handles compilation/execution internally.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Standard 9P operations (simplified) */
static int p9_attach(int sock_fd, const char *mount_point) {
    printf("[CLIENT] Attaching to %s\n", mount_point);
    return 0; /* Success */
}

static int p9_walk(int sock_fd, const char *path) {
    printf("[CLIENT] Walking to %s\n", path);
    return 0; /* Success */
}

static int p9_open(int sock_fd, const char *path, int mode) {
    printf("[CLIENT] Opening %s with mode %d\n", path, mode);
    return 0; /* Success */
}

static ssize_t p9_read(int sock_fd, void *buf, size_t count) {
    printf("[CLIENT] Reading %zu bytes\n", count);
    return count; /* Simulate read */
}

static ssize_t p9_write(int sock_fd, const void *buf, size_t count) {
    printf("[CLIENT] Writing %zu bytes\n", count);
    return count; /* Success */
}

static int p9_close(int sock_fd) {
    printf("[CLIENT] Closing connection\n");
    return 0; /* Success */
}

/* WASM file operations */
typedef struct {
    char *name;
    unsigned char *data;
    size_t size;
} WasmModule;

/* Example WASM module (simple) */
static unsigned char hello_wasm[] = {
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,  // WASM header
    0x01, 0x07, 0x01, 0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
    0x03, 0x02, 0x01, 0x00,
    0x07, 0x07, 0x01, 0x03, 0x68, 0x65, 0x6C, 0x6C,  // "hello"
    0x00, 0x00,
    0x0A, 0x09, 0x01, 0x07, 0x00, 0x41, 0x2A, 0x0B,
    0x7F, 0x00, 0x41, 0x2A, 0x0B, 0x7F, 0x00, 0x0B
};

static unsigned char factorial_wasm[] = {
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,  // WASM header
    0x01, 0x07, 0x01, 0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
    0x03, 0x02, 0x01, 0x00,
    0x07, 0x0A, 0x01, 0x06, 0x66, 0x61, 0x63, 0x74,  // "factorial"
    0x6F, 0x72, 0x69, 0x61, 0x6C, 0x00, 0x00,
    0x0A, 0x15, 0x02, 0x07, 0x00, 0x41, 0x84, 0x03,
    0x7F, 0x41, 0x80, 0x88, 0x01, 0x6A, 0x0B, 0x07,
    0x00, 0x41, 0x81, 0x01, 0x41, 0x80, 0x88, 0x01,
    0x6A, 0x0B
};

/* Example 1: Browse Available WASM Modules */
static int example_browse_modules(void) {
    printf("\n=== Example 1: Browse Available WASM Modules ===\n");
    
    int sock = 0; /* Placeholder for 9P socket */
    
    /* Step 1: Attach to WASM namespace */
    p9_attach(sock, "/wasm");
    
    /* Step 2: Walk to modules directory */
    p9_walk(sock, "/wasm/modules");
    
    /* Step 3: Read directory contents */
    char modules[1024];
    ssize_t n = p9_read(sock, modules, sizeof(modules));
    printf("[CLIENT] Available modules:\n%s", modules);
    
    /* Example output:
     * Available modules:
     * hello.wasm
     * factorial.wasm
     * calculator.wasm
     */
    
    p9_close(sock);
    return 0;
}

/* Example 2: Upload and Compile New WASM Module */
static int example_compile_module(void) {
    printf("\n=== Example 2: Upload and Compile WASM Module ===\n");
    
    int sock = 0; /* Placeholder for 9P socket */
    WasmModule *module = malloc(sizeof(WasmModule));
    
    /* Step 1: Prepare WASM module data */
    module->data = hello_wasm;
    module->size = sizeof(hello_wasm);
    module->name = "hello.wasm";
    
    printf("[CLIENT] Compiling WASM module: %s (%zu bytes)\n", 
           module->name, module->size);
    
    /* Step 2: Write to compilation request file */
    char compile_path[256];
    snprintf(compile_path, sizeof(compile_path), 
             "/wasm/compile/request");
    
    p9_open(sock, compile_path, O_WRONLY);
    ssize_t written = p9_write(sock, module->data, module->size);
    printf("[CLIENT] Submitted compilation request: %zd bytes\n", written);
    
    /* Step 3: Read compilation status */
    char status[256];
    p9_read(sock, status, sizeof(status));
    printf("[CLIENT] Compilation status: %s\n", status);
    
    /* Step 4: Check if compiled version is available */
    char cache_path[256];
    snprintf(cache_path, sizeof(cache_path),
             "/wasm/cache/%s.compiled", module->name);
    
    p9_open(sock, cache_path, O_RDONLY);
    char compiled_data[4096];
    ssize_t compiled_size = p9_read(sock, compiled_data, sizeof(compiled_data));
    printf("[CLIENT] Compiled module available: %zd bytes\n", compiled_size);
    
    p9_close(sock);
    free(module);
    return 0;
}

/* Example 3: Execute WASM Module */
static int example_execute_module(void) {
    printf("\n=== Example 3: Execute WASM Module ===\n");
    
    int sock = 0; /* Placeholder for 9P socket */
    const char *module_name = "hello.wasm";
    
    /* Step 1: Create execution handle */
    char exec_path[256];
    snprintf(exec_path, sizeof(exec_path), 
             "/wasm/exec/%s", module_name);
    
    printf("[CLIENT] Creating execution handle: %s\n", exec_path);
    p9_open(sock, exec_path, O_RDWR);
    
    /* Step 2: Write input data (if module needs input) */
    char input_data[256] = "Hello from client!";
    ssize_t input_written = p9_write(sock, input_data, strlen(input_data));
    printf("[CLIENT] Wrote input data: %zd bytes\n", input_written);
    
    /* Step 3: Read execution result */
    char output[1024];
    ssize_t output_size = p9_read(sock, output, sizeof(output));
    printf("[CLIENT] Execution output (%zd bytes): %s\n", output_size, output);
    
    /* Step 4: Clean up execution handle */
    p9_close(sock);
    
    return 0;
}

/* Example 4: Module Management */
static int example_module_management(void) {
    printf("\n=== Example 4: WASM Module Management ===\n");
    
    int sock = 0; /* Placeholder for 9P socket */
    
    /* Step 1: Get module metadata */
    char module_path[256] = "/wasm/modules/hello.wasm";
    p9_open(sock, module_path, O_RDONLY);
    
    /* Step 2: Stat file to get metadata */
    struct stat st;
    /* stat() would give us: size, modification time, etc. */
    printf("[CLIENT] Module info:\n");
    printf("  Path: %s\n", module_path);
    printf("  Size: %ld bytes\n", st.st_size);
    printf("  Modified: %ld\n", st.st_mtime);
    
    /* Step 3: Read module content */
    unsigned char wasm_data[4096];
    ssize_t data_size = p9_read(sock, wasm_data, sizeof(wasm_data));
    printf("  Content: %zd bytes of WASM bytecode\n", data_size);
    
    /* Step 4: Check compilation status */
    char compiled_path[256];
    snprintf(compiled_path, sizeof(compiled_path),
             "/wasm/cache/hello.wasm.compiled");
    
    p9_open(sock, compiled_path, O_RDONLY);
    struct stat compiled_st;
    /* stat() on compiled version */
    printf("  Compiled version: %ld bytes, modified %ld\n", 
           compiled_st.st_size, compiled_st.st_mtime);
    
    p9_close(sock);
    return 0;
}

/* Example 5: Batch Compilation */
static int example_batch_compilation(void) {
    printf("\n=== Example 5: Batch Compilation ===\n");
    
    int sock = 0; /* Placeholder for 9P socket */
    
    WasmModule modules[] = {
        { .name = "hello.wasm", .data = hello_wasm, .size = sizeof(hello_wasm) },
        { .name = "factorial.wasm", .data = factorial_wasm, .size = sizeof(factorial_wasm) }
    };
    
    int num_modules = sizeof(modules) / sizeof(modules[0]);
    
    printf("[CLIENT] Batch compiling %d modules\n", num_modules);
    
    for (int i = 0; i < num_modules; i++) {
        printf("[CLIENT] Compiling %s...\n", modules[i].name);
        
        /* Submit compilation request */
        char compile_path[256];
        snprintf(compile_path, sizeof(compile_path),
                 "/wasm/compile/request");
        
        p9_open(sock, compile_path, O_WRONLY);
        p9_write(sock, modules[i].data, modules[i].size);
        
        /* Check status */
        char status[256];
        p9_read(sock, status, sizeof(status));
        printf("[CLIENT] %s: %s\n", modules[i].name, status);
        
        p9_close(sock);
    }
    
    printf("[CLIENT] Batch compilation completed\n");
    return 0;
}

/* Main function demonstrating all examples */
int main(int argc, char *argv[]) {
    printf("=== WASM Fileserver Client Examples ===\n");
    printf("Demonstrating standard 9P operations for WASM module management\n\n");
    
    /* Example 1: Browse available modules */
    example_browse_modules();
    
    /* Example 2: Compile new module */
    example_compile_module();
    
    /* Example 3: Execute module */
    example_execute_module();
    
    /* Example 4: Module management */
    example_module_management();
    
    /* Example 5: Batch compilation */
    example_batch_compilation();
    
    printf("\n=== All Examples Completed ===\n");
    printf("Key Points:\n");
    printf("• All operations use standard 9P file operations\n");
    printf("• WASM-specific semantics encoded in file paths and data\n");
    printf("• No custom IPC protocols needed\n");
    printf("• Resurrection server provides reliability and monitoring\n");
    printf("• Clean separation: 9P for file ops, WASM runtime for execution\n");
    
    return 0;
}
