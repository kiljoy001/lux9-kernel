/* simple_wasm_fileserver.c - Simplified WASM Fileserver Implementation
 *
 * This demonstrates the simplified architecture using standard 9P protocol
 * monitored by the resurrection server.
 *
 * Key principles:
 * - Standard 9P protocol implementation
 * - WASM operations as file operations
 * - NO custom IPC mechanisms
 * - Monitored by resurrection server for reliability
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <sys/wait.h>

/* Simple 9P message types (simplified) */
#define P9_Tversion 100
#define P9_Rversion 101
#define P9_Tattach 104
#define P9_Rattach 105
#define P9_Twalk 110
#define P9_Rwalk 111
#define P9_Topen 112
#define P9_Ropen 113
#define P9_Tcreate 114
#define P9_Rcreate 115
#define P9_Tread 116
#define P9_Rread 117
#define P9_Twrite 118
#define P9_Rwrite 119
#define P9_Tclunk 120
#define P9_Rclunk 121
#define P9_Tstat 124
#define P9_Rstat 125

/* WASM namespace root */
#define WASM_ROOT "/wasm"
#define MODULES_DIR "/wasm/modules"
#define CACHE_DIR "/wasm/cache"
#define EXEC_DIR "/wasm/exec"
#define COMPILE_DIR "/wasm/compile"

/* Global state */
static int running = 1;
static pid_t my_pid;

/* WASM module cache */
typedef struct {
    char name[256];
    char path[512];
    unsigned char hash[32];  /* SHA256 hash */
    time_t last_modified;
    int compiled;
    char compiled_path[512];
} WasmModule;

#define MAX_MODULES 256
static WasmModule modules[MAX_MODULES];
static int num_modules = 0;

/* Compilation cache */
typedef struct {
    unsigned char hash[32];
    char compiled_path[512];
    time_t created;
    int valid;
} CompiledCache;

#define MAX_CACHE 1024
static CompiledCache cache[MAX_CACHE];
static int cache_size = 0;

/* Simple file operations */
static int do_open(const char *path, int mode) {
    return open(path, mode);
}

static int do_close(int fd) {
    return close(fd);
}

static ssize_t do_read(int fd, void *buf, size_t count) {
    return read(fd, buf, count);
}

static ssize_t do_write(int fd, const void *buf, size_t count) {
    return write(fd, buf, count);
}

static int do_stat(const char *path, struct stat *st) {
    return stat(path, st);
}

/* Print function for logging */
static void print(const char *msg) {
    time_t now = time(NULL);
    printf("[%ld] WASM-FS [PID %d]: %s", now, my_pid, msg);
    fflush(stdout);
}

static void print_fmt(const char *fmt, ...) {
    va_list args;
    time_t now = time(NULL);
    printf("[%ld] WASM-FS [PID %d]: ", now, my_pid);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}

/* Initialize WASM namespace directories */
static int init_wasm_namespace(void) {
    print("Initializing WASM namespace...\n");
    
    /* Create directories */
    system("mkdir -p " MODULES_DIR);
    system("mkdir -p " CACHE_DIR);
    system("mkdir -p " EXEC_DIR);
    system("mkdir -p " COMPILE_DIR);
    
    /* Scan for existing WASM modules */
    DIR *dir = opendir(MODULES_DIR);
    if (!dir) {
        print("Failed to open modules directory\n");
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && num_modules < MAX_MODULES) {
        if (strstr(entry->d_name, ".wasm")) {
            snprintf(modules[num_modules].name, sizeof(modules[num_modules].name), 
                    "%s", entry->d_name);
            snprintf(modules[num_modules].path, sizeof(modules[num_modules].path),
                    "%s/%s", MODULES_DIR, entry->d_name);
            
            /* Compute hash */
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "sha256sum %s 2>/dev/null | cut -d' ' -f1", 
                    modules[num_modules].path);
            FILE *fp = popen(cmd, "r");
            if (fp) {
                char hash_str[64];
                if (fgets(hash_str, sizeof(hash_str), fp)) {
                    /* Convert hex to binary */
                    for (int i = 0; i < 32; i++) {
                        sscanf(hash_str + i*2, "%02hhx", &modules[num_modules].hash[i]);
                    }
                }
                pclose(fp);
            }
            
            /* Check if compiled version exists */
            snprintf(modules[num_modules].compiled_path, 
                    sizeof(modules[num_modules].compiled_path),
                    "%s/%s.compiled", CACHE_DIR, entry->d_name);
            struct stat st;
            if (stat(modules[num_modules].compiled_path, &st) == 0) {
                modules[num_modules].compiled = 1;
            }
            
            print_fmt("Found module: %s (hash: %32ph)\n", 
                     entry->d_name, modules[num_modules].hash);
            num_modules++;
        }
    }
    
    closedir(dir);
    print_fmt("Initialized %d WASM modules\n", num_modules);
    return 0;
}

/* Handle Tread - Read file content */
static int handle_tread(int fd, const char *path, void *buf, size_t count) {
    print_fmt("Tread: %s (%zu bytes)\n", path, count);
    
    /* Handle directory reads */
    if (strcmp(path, WASM_ROOT) == 0) {
        const char *dir_content = "modules/\ncache/\nexec/\ncompile/\n";
        size_t len = strlen(dir_content);
        if (count < len) len = count;
        memcpy(buf, dir_content, len);
        return len;
    }
    
    if (strcmp(path, MODULES_DIR) == 0) {
        char dir_content[1024] = "";
        int pos = 0;
        for (int i = 0; i < num_modules; i++) {
            pos += snprintf(dir_content + pos, sizeof(dir_content) - pos,
                          "%s\n", modules[i].name);
        }
        size_t len = strlen(dir_content);
        if (count < len) len = count;
        memcpy(buf, dir_content, len);
        return len;
    }
    
    /* Handle module reads */
    if (strstr(path, MODULES_DIR) && strstr(path, ".wasm")) {
        int module_fd = do_open(path, O_RDONLY);
        if (module_fd < 0) {
            print_fmt("Failed to open module: %s\n", path);
            return -1;
        }
        
        ssize_t n = do_read(module_fd, buf, count);
        do_close(module_fd);
        return n;
    }
    
    /* Handle cache reads */
    if (strstr(path, CACHE_DIR) && strstr(path, ".compiled")) {
        int cache_fd = do_open(path, O_RDONLY);
        if (cache_fd < 0) {
            print_fmt("Failed to open cache: %s\n", path);
            return -1;
        }
        
        ssize_t n = do_read(cache_fd, buf, count);
        do_close(cache_fd);
        return n;
    }
    
    /* Handle execution results */
    if (strstr(path, EXEC_DIR)) {
        const char *result = "WASM execution result: Hello, World!\n";
        size_t len = strlen(result);
        if (count < len) len = count;
        memcpy(buf, result, len);
        return len;
    }
    
    print_fmt("Unknown read path: %s\n", path);
    return -1;
}

/* Handle Twrite - Write file content */
static int handle_twrite(int fd, const char *path, const void *buf, size_t count) {
    print_fmt("Twrite: %s (%zu bytes)\n", path, count);
    
    /* Handle compilation requests */
    if (strcmp(path, COMPILE_DIR "/request") == 0) {
        print("Compilation request received\n");
        
        /* Write to temporary file */
        char temp_file[] = "/tmp/wasm_compile_XXXXXX.wasm";
        int temp_fd = mkstemp(temp_file);
        if (temp_fd < 0) {
            print("Failed to create temp file\n");
            return -1;
        }
        
        do_write(temp_fd, buf, count);
        
        /* Compile WASM module */
        char compiled_file[512];
        snprintf(compiled_file, sizeof(compiled_file),
                "%s/%s.compiled", CACHE_DIR, "temp_module");
        
        /* Simple compilation: just copy to cache for demo */
        int compiled_fd = do_open(compiled_file, O_WRONLY | O_CREAT, 0644);
        if (compiled_fd < 0) {
            print("Failed to create compiled file\n");
            close(temp_fd);
            unlink(temp_file);
            return -1;
        }
        
        /* Copy content */
        lseek(temp_fd, 0, SEEK_SET);
        char temp_buf[4096];
        ssize_t n;
        while ((n = do_read(temp_fd, temp_buf, sizeof(temp_buf))) > 0) {
            do_write(compiled_fd, temp_buf, n);
        }
        
        do_close(compiled_fd);
        close(temp_fd);
        unlink(temp_file);
        
        print("Compilation completed\n");
        return count;
    }
    
    /* Handle module uploads */
    if (strstr(path, MODULES_DIR) && strstr(path, ".wasm")) {
        print_fmt("Module upload to: %s\n", path);
        
        int module_fd = do_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (module_fd < 0) {
            print_fmt("Failed to create module: %s\n", path);
            return -1;
        }
        
        ssize_t n = do_write(module_fd, buf, count);
        do_close(module_fd);
        
        print("Module uploaded successfully\n");
        return n;
    }
    
    print_fmt("Unknown write path: %s\n", path);
    return -1;
}

/* Handle Tstat - Get file metadata */
static int handle_tstat(const char *path, struct stat *st) {
    print_fmt("Tstat: %s\n", path);
    
    if (do_stat(path, st) == 0) {
        return 0;
    }
    
    /* For virtual WASM paths, return synthetic stats */
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFREG | 0644;
    st->st_size = 1024;  /* Default size */
    st->st_mtime = time(NULL);
    st->st_uid = getuid();
    st->st_gid = getgid();
    
    return 0;
}

/* Simple 9P message dispatcher */
static void dispatch_9p_message(int client_fd) {
    char buffer[4096];
    ssize_t n = do_read(client_fd, buffer, sizeof(buffer));
    
    if (n <= 0) {
        return;
    }
    
    /* Parse simplified 9P header */
    unsigned char *p = (unsigned char *)buffer;
    unsigned int size = *(unsigned int*)p;
    unsigned char type = p[4];
    unsigned short tag = *(unsigned short*)(p + 5);
    
    print_fmt("9P message: type=%d, tag=%d, size=%u\n", type, tag, size);
    
    char response[4096];
    int resp_size = 0;
    
    switch (type) {
    case P9_Tversion:
        /* Handle version negotiation */
        print("Tversion received\n");
        /* Respond with Rversion */
        resp_size = snprintf(response, sizeof(response),
                           "Version response");
        break;
        
    case P9_Tattach:
        /* Handle mount attach */
        print("Tattach received\n");
        resp_size = snprintf(response, sizeof(response),
                           "Attach response");
        break;
        
    case P9_Twalk:
        /* Handle path walking */
        print("Twalk received\n");
        resp_size = snprintf(response, sizeof(response),
                           "Walk response");
        break;
        
    case P9_Topen:
        /* Handle file open */
        print("Topen received\n");
        resp_size = snprintf(response, sizeof(response),
                           "Open response");
        break;
        
    case P9_Tread:
        /* Handle file read */
        {
            char path[256] = WASM_ROOT;  /* Simplified: assume root */
            resp_size = handle_tread(client_fd, path, 
                                   response + 16, sizeof(response) - 16);
            if (resp_size >= 0) {
                /* Build response */
                resp_size += 16;
            }
        }
        break;
        
    case P9_Twrite:
        /* Handle file write */
        {
            char path[256] = WASM_ROOT;  /* Simplified: assume root */
            resp_size = handle_twrite(client_fd, path,
                                    buffer + 16, n - 16);
        }
        break;
        
    case P9_Tstat:
        /* Handle file status */
        {
            char path[256] = WASM_ROOT;  /* Simplified: assume root */
            struct stat st;
            if (handle_tstat(path, &st) == 0) {
                resp_size = snprintf(response, sizeof(response),
                                   "Stat: size=%ld, mode=%o\n",
                                   st.st_size, st.st_mode);
            }
        }
        break;
        
    case P9_Tclunk:
        /* Handle file close */
        print("Tclunk received\n");
        resp_size = snprintf(response, sizeof(response),
                           "Clunk response");
        break;
        
    default:
        print_fmt("Unknown 9P message type: %d\n", type);
        resp_size = snprintf(response, sizeof(response),
                           "Unknown operation");
        break;
    }
    
    if (resp_size > 0) {
        do_write(client_fd, response, resp_size);
    }
}

/* Health check for resurrection server */
static void health_check(void) {
    /* Create/verify health check file */
    char health_file[256];
    snprintf(health_file, sizeof(health_file),
            "/tmp/wasm_fileserver_health_%d", my_pid);
    
    FILE *f = fopen(health_file, "w");
    if (f) {
        fprintf(f, "PID: %d\n", my_pid);
        fprintf(f, "Time: %ld\n", time(NULL));
        fprintf(f, "Modules: %d\n", num_modules);
        fclose(f);
    }
}

/* Signal handler for clean shutdown */
static void signal_handler(int sig) {
    print_fmt("Received signal %d, shutting down...\n", sig);
    running = 0;
}

int main(int argc, char *argv[]) {
    my_pid = getpid();
    
    print("=== Simple WASM Fileserver Starting ===\n");
    print_fmt("PID: %d\n", my_pid);
    
    /* Setup signal handlers */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    /* Initialize WASM namespace */
    if (init_wasm_namespace() < 0) {
        print("Failed to initialize WASM namespace\n");
        return 1;
    }
    
    /* Create simple 9P server socket */
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        print("Failed to create socket\n");
        return 1;
    }
    
    /* Bind to WASM namespace socket */
    unlink("/tmp/wasm_fileserver.sock");
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/wasm_fileserver.sock", 
           sizeof(addr.sun_path) - 1);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        print("Failed to bind socket\n");
        return 1;
    }
    
    if (listen(server_fd, 5) < 0) {
        print("Failed to listen\n");
        return 1;
    }
    
    print("Listening on /tmp/wasm_fileserver.sock\n");
    
    /* Main server loop */
    time_t last_health_check = 0;
    
    while (running) {
        /* Accept client connections */
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            print("Accept failed\n");
            continue;
        }
        
        /* Handle client request */
        dispatch_9p_message(client_fd);
        do_close(client_fd);
        
        /* Periodic health check */
        time_t now = time(NULL);
        if (now - last_health_check > 10) {
            health_check();
            last_health_check = now;
        }
    }
    
    print("Shutting down...\n");
    do_close(server_fd);
    unlink("/tmp/wasm_fileserver.sock");
    unlink("/tmp/wasm_fileserver_health");
    
    print("WASM fileserver stopped\n");
    return 0;
}
