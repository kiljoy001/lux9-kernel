/*
 * Simple 9P WASM Fileserver - Standalone Userspace Implementation
 * 
 * This demonstrates the simplified architecture:
 * - Standard 9P protocol
 * - Filesystem-based WASM operations  
 * - Resurrection server monitoring
 * - Uses existing WASI rights system (referenced but not included)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>

/* Basic types for compatibility */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef void *uintptr;

/* WASI Rights (would be defined in actual implementation) */
#define WASI_RIGHT_FD_READ     (1ULL << 1)
#define WASI_RIGHT_FD_WRITE    (1ULL << 6)
#define WASI_RIGHT_FD_READDIR  (1ULL << 14)
#define WASI_RIGHT_PATH_OPEN   (1ULL << 13)
#define WASI_RIGHT_PATH_CREATE_FILE (1ULL << 10)
#define WASI_RIGHT_PATH_UNLINK_FILE (1ULL << 16)

/* Fileserver rights mapping */
#define FILESERVER_READ     (WASI_RIGHT_FD_READ | WASI_RIGHT_PATH_OPEN)
#define FILESERVER_WRITE    (WASI_RIGHT_FD_WRITE | WASI_RIGHT_PATH_CREATE_FILE)
#define FILESERVER_ADMIN    (WASI_RIGHT_FD_WRITE | WASI_RIGHT_PATH_UNLINK_FILE)
#define FILESERVER_COMPILE  (WASI_RIGHT_FD_WRITE | WASI_RIGHT_PATH_CREATE_FILE)

/* WASI Context - simplified version */
typedef struct {
    int fds[32];              // File descriptors
    int fds_rights[32];       // Rights for each FD
    bool fds_open[32];        // Open status
} WasiContext;

/* Fileserver context */
typedef struct {
    WasiContext wasi;
    char root_path[256];
    volatile bool running;
} WasmFileserverContext;

/* File information */
typedef struct {
    char name[256];
    char path[512];
    bool is_dir;
    u64 size;
    time_t mtime;
    mode_t mode;
} WasmFile;

/* Simple 9P message types */
typedef enum {
    P9_TATTACH = 104, P9_TSTAT = 108, P9_TWALK = 110,
    P9_TOPEN = 114, P9_TCREATE = 115, P9_TREAD = 116,
    P9_TWRITE = 117, P9_TCLUNK = 120, P9_TREMOVE = 121,
    P9_TWASM_COMPILE = 200, P9_TWASM_EXECUTE = 201
} P9Type;

/* Request/Response structures */
typedef struct {
    P9Type type;
    u32 tag;
    u32 fid;
    char path[512];
    u64 offset;
    u32 count;
    void *data;
} P9Request;

typedef struct {
    P9Type type;
    u32 tag;
    int status;
    char error[256];
    WasmFile file;
    u32 count;
} P9Response;

/* Global context */
static WasmFileserverContext *g_ctx = NULL;

/* Initialization Functions */

/* Initialize WASI context with fileserver capabilities */
static int init_wasi_context(WasmFileserverContext *ctx) {
    memset(&ctx->wasi, 0, sizeof(WasiContext));
    
    /* Initialize standard FDs */
    for (int i = 0; i < 32; i++) {
        ctx->wasi.fds[i] = -1;
        ctx->wasi.fds_rights[i] = 0;
        ctx->wasi.fds_open[i] = false;
    }
    
    /* stdin */
    ctx->wasi.fds[0] = STDIN_FILENO;
    ctx->wasi.fds_rights[0] = WASI_RIGHT_FD_READ;
    ctx->wasi.fds_open[0] = true;
    
    /* stdout */
    ctx->wasi.fds[1] = STDOUT_FILENO;
    ctx->wasi.fds_rights[1] = WASI_RIGHT_FD_WRITE;
    ctx->wasi.fds_open[1] = true;
    
    /* stderr */
    ctx->wasi.fds[2] = STDERR_FILENO;
    ctx->wasi.fds_rights[2] = WASI_RIGHT_FD_WRITE;
    ctx->wasi.fds_open[2] = true;
    
    /* WASM root directory */
    ctx->wasi.fds[3] = open("/wasm", O_RDONLY | O_DIRECTORY);
    if (ctx->wasi.fds[3] < 0) {
        fprintf(stderr, "Failed to open /wasm directory\n");
        return -1;
    }
    ctx->wasi.fds_rights[3] = FILESERVER_READ | FILESERVER_WRITE | FILESERVER_ADMIN;
    ctx->wasi.fds_open[3] = true;
    
    printf("WASI context initialized\n");
    return 0;
}

/* Create WASM namespace structure */
static int create_wasm_namespace(void) {
    const char *dirs[] = {
        "/wasm",
        "/wasm/modules",
        "/wasm/cache", 
        "/wasm/exec",
        "/wasm/compile",
        NULL
    };
    
    for (int i = 0; dirs[i]; i++) {
        if (mkdir(dirs[i], 0755) < 0 && errno != EEXIST) {
            fprintf(stderr, "Failed to create directory %s: %s\n", dirs[i], strerror(errno));
            return -1;
        }
        printf("Created directory: %s\n", dirs[i]);
    }
    
    return 0;
}

/* Validation Functions */

/* Simple path validation */
static bool is_path_valid(const char *path) {
    if (!path || strlen(path) == 0 || path[0] != '/') {
        return false;
    }
    
    /* Prevent path traversal */
    if (strstr(path, "..") != NULL) {
        return false;
    }
    
    /* Ensure path stays within /wasm */
    if (strncmp(path, "/wasm", 5) != 0 && strcmp(path, "/wasm") != 0) {
        return false;
    }
    
    return true;
}

/* Simulate WASI rights checking */
static bool check_wasi_rights(WasmFileserverContext *ctx, int fd, u64 required_rights) {
    if (fd < 0 || fd >= 32 || !ctx->wasi.fds_open[fd]) {
        return false;
    }
    
    /* Check if process has required rights */
    u64 available_rights = ctx->wasi.fds_rights[fd];
    return (available_rights & required_rights) == required_rights;
}

/* 9P Operation Handlers */

/* Handle directory walk */
static int handle_walk(P9Request *req, P9Response *resp) {
    printf("Processing TWALK: %s\n", req->path);
    
    if (!is_path_valid(req->path)) {
        resp->status = -1;
        strcpy(resp->error, "Invalid path");
        return -1;
    }
    
    /* Check directory read permissions */
    if (!check_wasi_rights(g_ctx, 3, FILESERVER_READ)) {
        resp->status = -1;
        strcpy(resp->error, "Permission denied");
        return -1;
    }
    
    /* Build absolute path */
    char abs_path[512];
    snprintf(abs_path, sizeof(abs_path), "%s%s", g_ctx->root_path, req->path);
    
    /* List directory */
    DIR *dir = opendir(abs_path);
    if (!dir) {
        resp->status = -1;
        strcpy(resp->error, "Directory not found");
        return -1;
    }
    
    /* Read directory entries */
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL && count < req->count) {
        strcpy(resp->file.name, entry->d_name);
        resp->file.is_dir = (entry->d_type == DT_DIR);
        
        /* Get file stats */
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", abs_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            resp->file.size = st.st_size;
            resp->file.mtime = st.st_mtime;
            resp->file.mode = st.st_mode;
        } else {
            resp->file.size = 0;
            resp->file.mtime = time(NULL);
            resp->file.mode = 0644;
        }
        
        count++;
        resp->count = count;
    }
    
    closedir(dir);
    resp->status = 0;
    
    printf("TWALK completed: %d entries\n", count);
    return 0;
}

/* Handle file read */
static int handle_read(P9Request *req, P9Response *resp) {
    printf("Processing TREAD: %s offset=%llu count=%u\n", req->path, req->offset, req->count);
    
    if (!is_path_valid(req->path)) {
        resp->status = -1;
        strcpy(resp->error, "Invalid path");
        return -1;
    }
    
    /* Check read permissions */
    if (!check_wasi_rights(g_ctx, 3, FILESERVER_READ)) {
        resp->status = -1;
        strcpy(resp->error, "Permission denied");
        return -1;
    }
    
    /* Build absolute path */
    char abs_path[512];
    snprintf(abs_path, sizeof(abs_path), "%s%s", g_ctx->root_path, req->path);
    
    /* Open file */
    int fd = open(abs_path, O_RDONLY);
    if (fd < 0) {
        resp->status = -1;
        strcpy(resp->error, "File not found");
        return -1;
    }
    
    /* Seek to offset and read */
    if (lseek(fd, req->offset, SEEK_SET) < 0) {
        close(fd);
        resp->status = -1;
        strcpy(resp->error, "Seek failed");
        return -1;
    }
    
    /* Read data */
    resp->count = read(fd, req->data, req->count);
    close(fd);
    
    if (resp->count < 0) {
        resp->status = -1;
        strcpy(resp->error, "Read failed");
        return -1;
    }
    
    resp->status = 0;
    printf("TREAD completed: %u bytes\n", resp->count);
    return 0;
}

/* Handle file write */
static int handle_write(P9Request *req, P9Response *resp) {
    printf("Processing TWRITE: %s offset=%llu count=%u\n", req->path, req->offset, req->count);
    
    if (!is_path_valid(req->path)) {
        resp->status = -1;
        strcpy(resp->error, "Invalid path");
        return -1;
    }
    
    /* Check write permissions */
    if (!check_wasi_rights(g_ctx, 3, FILESERVER_WRITE)) {
        resp->status = -1;
        strcpy(resp->error, "Permission denied");
        return -1;
    }
    
    /* Build absolute path */
    char abs_path[512];
    snprintf(abs_path, sizeof(abs_path), "%s%s", g_ctx->root_path, req->path);
    
    /* Open file for writing */
    int fd = open(abs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        resp->status = -1;
        strcpy(resp->error, "Cannot create file");
        return -1;
    }
    
    /* Seek to offset and write */
    if (lseek(fd, req->offset, SEEK_SET) < 0) {
        close(fd);
        resp->status = -1;
        strcpy(resp->error, "Seek failed");
        return -1;
    }
    
    /* Write data */
    resp->count = write(fd, req->data, req->count);
    close(fd);
    
    if (resp->count < 0) {
        resp->status = -1;
        strcpy(resp->error, "Write failed");
        return -1;
    }
    
    resp->status = 0;
    printf("TWRITE completed: %u bytes\n", resp->count);
    return 0;
}

/* Handle WASM compilation */
static int handle_wasm_compile(P9Request *req, P9Response *resp) {
    printf("Processing TWASM_COMPILE: %s\n", req->path);
    
    if (!is_path_valid(req->path)) {
        resp->status = -1;
        strcpy(resp->error, "Invalid path");
        return -1;
    }
    
    /* Check compilation permissions */
    if (!check_wasi_rights(g_ctx, 3, FILESERVER_COMPILE)) {
        resp->status = -1;
        strcpy(resp->error, "Compilation permission denied");
        return -1;
    }
    
    /* Build paths */
    char module_path[512];
    char cache_path[512];
    snprintf(module_path, sizeof(module_path), "%s/modules%s", g_ctx->root_path, req->path);
    snprintf(cache_path, sizeof(cache_path), "%s/cache%s.compiled", g_ctx->root_path, req->path);
    
    /* Check if WASM module exists */
    struct stat st;
    if (stat(module_path, &st) < 0) {
        resp->status = -1;
        strcpy(resp->error, "WASM module not found");
        return -1;
    }
    
    /* Simulate compilation */
    printf("Compiling WASM module: %s (%lld bytes)\n", module_path, st.st_size);
    
    /* Create cache entry */
    FILE *cache_f = fopen(cache_path, "w");
    if (!cache_f) {
        resp->status = -1;
        strcpy(resp->error, "Cannot create cache file");
        return -1;
    }
    
    fprintf(cache_f, "# WASM Compiled Cache Entry\n");
    fprintf(cache_f, "Module: %s\n", req->path);
    fprintf(cache_f, "Size: %lld bytes\n", st.st_size);
    fprintf(cache_f, "Compiled: %s\n", ctime(&(time_t){time(NULL)}));
    fprintf(cache_f, "Status: SUCCESS\n");
    
    fclose(cache_f);
    
    /* Return compilation result */
    const char *result = "WASM module compiled successfully";
    resp->count = strlen(result);
    if (req->data && req->count >= resp->count) {
        strcpy((char*)req->data, result);
    }
    
    resp->status = 0;
    printf("TWASM_COMPILE completed: %s\n", req->path);
    return 0;
}

/* Signal handling */
static void signal_handler(int sig) {
    printf("\nWASM fileserver: Received signal %d, shutting down...\n", sig);
    if (g_ctx) {
        g_ctx->running = false;
    }
}

/* Health check for resurrection server */
static int health_check(void) {
    /* Check if filesystem is accessible */
    if (access(g_ctx->root_path, F_OK) != 0) {
        return -1;
    }
    
    /* Check WASI context */
    if (!g_ctx->wasi.fds_open[0] || !g_ctx->wasi.fds_open[1] || !g_ctx->wasi.fds_open[2]) {
        return -1;
    }
    
    return 0;
}

/* Main processing loop */
static int process_requests(void) {
    printf("WASM fileserver: Starting request processing loop\n");
    
    while (g_ctx && g_ctx->running) {
        /* Simple request handling - in real implementation would use proper 9P protocol */
        
        /* Read command from stdin */
        char line[1024];
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (feof(stdin)) {
                printf("EOF detected, exiting\n");
                break;
            }
            if (errno == EINTR) continue;
            fprintf(stderr, "Error reading input: %s\n", strerror(errno));
            break;
        }
        
        /* Parse simple command format: COMMAND PATH [offset count] */
        P9Request req = {0};
        P9Response resp = {0};
        resp.type = 0;
        resp.status = -1;
        resp.count = 0;
        resp.error[0] = '\0';
        
        /* Parse command */
        if (sscanf(line, "WALK %s", req.path) == 1) {
            req.type = P9_TWALK;
            req.count = 10; // Max entries
            handle_walk(&req, &resp);
        } else if (sscanf(line, "READ %s %llu %u", req.path, &req.offset, &req.count) == 3) {
            req.type = P9_TREAD;
            req.data = malloc(req.count);
            if (req.data) {
                handle_read(&req, &resp);
                free(req.data);
            } else {
                resp.status = -1;
                strcpy(resp.error, "Out of memory");
            }
        } else if (sscanf(line, "WRITE %s %llu %u", req.path, &req.offset, &req.count) == 3) {
            req.type = P9_TWRITE;
            req.data = malloc(req.count);
            if (req.data) {
                /* Read data from stdin */
                size_t actual = fread(req.data, 1, req.count, stdin);
                req.count = actual;
                handle_write(&req, &resp);
                free(req.data);
            } else {
                resp.status = -1;
                strcpy(resp.error, "Out of memory");
            }
    } else if (sscanf(line, "COMPILE %s", req.path) == 1) {
        req.type = P9_TWASM_COMPILE;
        req.data = malloc(256);
        req.count = 256;
        handle_wasm_compile(&req, &resp);
        free(req.data);
        } else if (strcmp(line, "HEALTH\n") == 0 || strcmp(line, "HEALTH\r\n") == 0) {
            /* Health check command */
            if (health_check() == 0) {
                printf("HEALTHY\n");
            } else {
                printf("UNHEALTHY\n");
            }
            continue;
        } else if (strcmp(line, "QUIT\n") == 0 || strcmp(line, "QUIT\r\n") == 0) {
            printf("WASM fileserver: Quit command received\n");
            break;
        } else if (strlen(line) > 1) {
            /* Unknown command */
            fprintf(stderr, "Unknown command: %s", line);
            resp.status = -1;
            strcpy(resp.error, "Unknown command");
        } else {
            continue; // Skip empty lines
        }
        
        /* Send response */
        if (resp.status == 0) {
            printf("SUCCESS: %u bytes\n", resp.count);
            if (resp.count > 0 && resp.type == P9_TWASM_COMPILE) {
                printf("%s\n", (char*)req.data);
            }
        } else {
            printf("ERROR: %s\n", resp.error);
        }
    }
    
    return 0;
}

/* Main function */
int main(int argc, char **argv) {
    printf("=== Simple 9P WASM Fileserver ===\n");
    printf("Architecture: Simple 9P server monitored by resurrection server\n");
    printf("Capabilities: Uses existing WASI rights system\n");
    printf("Namespace: /wasm/\n");
    printf("=====================================\n\n");
    
    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Allocate context */
    g_ctx = malloc(sizeof(WasmFileserverContext));
    if (!g_ctx) {
        fprintf(stderr, "Failed to allocate fileserver context\n");
        return 1;
    }
    
    /* Initialize */
    strcpy(g_ctx->root_path, "/wasm");
    g_ctx->running = true;
    
    printf("Initializing WASI context...\n");
    if (init_wasi_context(g_ctx) < 0) {
        fprintf(stderr, "Failed to initialize WASI context\n");
        free(g_ctx);
        return 1;
    }
    
    printf("Creating WASM namespace...\n");
    if (create_wasm_namespace() < 0) {
        fprintf(stderr, "Failed to create WASM namespace\n");
        free(g_ctx);
        return 1;
    }
    
    printf("\n=== Fileserver Ready ===\n");
    printf("Root: %s\n", g_ctx->root_path);
    printf("Rights: FILESERVER_READ | FILESERVER_WRITE | FILESERVER_ADMIN\n");
    printf("Capabilities: WASI rights system\n");
    printf("Available commands: WALK, READ, WRITE, COMPILE, HEALTH, QUIT\n\n");
    
    /* Main processing loop */
    process_requests();
    
    /* Cleanup */
    printf("\nWASM fileserver: Shutdown complete\n");
    free(g_ctx);
    
    return 0;
}