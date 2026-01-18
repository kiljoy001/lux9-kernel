#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// Rename standard functions that resurrection.c redefines as static
#define strcmp my_strcmp
#define strlen my_strlen
#define strncpy my_strncpy
#define memset my_memset
#define strcpy my_strcpy

long long nsec(void);

#define UNIT_TEST
#include "resurrection.c"

// Undefine them so we can use standard functions
#undef strcmp
#undef strlen
#undef strncpy
#undef memset
#undef strcpy

// --- Mock State ---
int last_rfork_pid = 0;
int next_pid = 200;
char last_exec_path[256];
int mock_wait_pid = 0; 

// --- Mock Syscalls ---

int sys_open(char *path, int mode) {
    printf("[MOCK] sys_open(%s, %d)\n", path, mode);
    return 10;
}

int sys_close(int fd) {
    return 0;
}

long sys_read(int fd, void *buf, long n) {
    return 0; // EOF
}

long sys_write(int fd, void *buf, long n) {
    if (n > 0) {
        // fwrite(buf, 1, n, stdout); 
    }
    return n;
}

void sys_exit(char *msg) {
    printf("[MOCK] sys_exit(%s)\n", msg);
    exit(0);
}

int sys_create(char *path, int mode, uint perm) {
    printf("[MOCK] sys_create(%s)\n", path);
    return 11;
}

int sys_rfork(int flags) {
    last_rfork_pid = next_pid++;
    printf("[MOCK] sys_rfork() -> %d (Parent logic)\n", last_rfork_pid);
    return last_rfork_pid; 
}

void sys_exec(char *path) {
    printf("[MOCK] sys_exec(%s)\n", path);
    strncpy(last_exec_path, path, sizeof(last_exec_path) - 1);
}

int sys_pipe(int *fds) {
    fds[0] = 20;
    fds[1] = 21;
    return 0;
}

int sys_wait(void) {
    if (mock_wait_pid > 0) {
        int p = mock_wait_pid;
        mock_wait_pid = 0;
        printf("[MOCK] sys_wait() -> %d\n", p);
        return p;
    }
    return -1;
}

int sys_mount(int fd, int afd, char *old, int flags, char *aname) {
    printf("[MOCK] sys_mount(%s)\n", old);
    return 0;
}

// --- Mock Crypto/Caps ---

int cap_epoch_valid(u64int e, u64int current, u64int grace) { return 1; }
int cap_token_verify(const CapToken *tok, const u8int *service_hash, const u8int *pubkey, u64int current_epoch) { return 0; }
int cap_blind_sign(CapBlindResponse *resp, const CapBlindRequest *req, const u8int *privkey) { return 0; }
void crypto_eddsa_key_pair(u8int *priv, u8int *pub, u8int *seed) {}
u64int cap_current_epoch(int duration) { return 100; }

// Hash mocks
void crypto_blake2b_init(void *ctx, unsigned long long outlen) {}
void crypto_blake2b_update(void *ctx, const u8int *in, unsigned long long inlen) {}
void crypto_blake2b_final(void *ctx, u8int *out) {
    memset(out, 0, 32); 
}

// --- Replaced IO Functions ---

int print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}

void print_num(const char *prefix, int num, const char *suffix) {
    printf("%s%d%s", prefix, num, suffix ? suffix : "");
}

long long nsec(void) {
    // Return a dummy time (1 second)
    return 1000000000;
}

// --- Tests ---

void test_sophia_registration() {
    printf("\n=== Test Case 1: Sophia Service Registration ===\n");
    my_memset(services, 0, sizeof(services));
    num_services = 0;
    
    int sophia_pid = 50;
    srv_create_entry("sophia", sophia_pid);
    
    if (num_services == 1 && strcmp(services[0].name, "sophia") == 0) {
        printf("PASS: Sophia registered with correct name\n");
    } else {
        printf("FAIL: Registration failed\n");
    }
    
    if (services[0].pid == 50) {
        printf("PASS: Sophia PID is 50\n");
    } else {
        printf("FAIL: Sophia PID is %d (Expected 50)\n", services[0].pid);
    }
}

void test_sophia_startup() {
    printf("\n=== Test Case 2: Sophia Startup ===\n");
    Service *s = &services[0];
    strcpy(s->exec_path, "/boot/sophia");
    s->state = SRV_STOPPED;
    s->auto_restart = 1;
    
    printf("Calling start_service...\n");
    start_service(s);
    
    if (s->state == SRV_RUNNING) {
        printf("PASS: State is RUNNING\n");
    } else {
        printf("FAIL: State is %d\n", s->state);
    }
    
    if (s->pid == last_rfork_pid) {
        printf("PASS: PID updated via rfork\n");
    } else {
        printf("FAIL: PID mismatch\n");
    }
}

void test_sophia_crash_and_recovery() {
    printf("\n=== Test Case 3: Sophia Recovery ===\n");
    Service *s = &services[0];
    int old_pid = s->pid;
    
    // Simulate crash logic
    s->state = SRV_CRASHED;
    s->pid = 0;
    
    printf("Simulating restart (auto-restart enabled)...\n");
    restart_service(s);
    
    if (s->restarts == 1) {
        printf("PASS: Restart count is 1\n");
    } else {
        printf("FAIL: Restart count is %d\n", s->restarts);
    }
    
    if (s->pid != old_pid && s->pid != 0) {
        printf("PASS: Service has new PID %d\n", s->pid);
    } else {
        printf("FAIL: Service PID issue. Old: %d, New: %d\n", old_pid, s->pid);
    }
    
    if (s->state == SRV_RUNNING) {
        printf("PASS: Service state is SRV_RUNNING\n");
    } else {
        printf("FAIL: Service state is %d\n", s->state);
    }
}

int main() {
    // Initialize mock exchange (though print no longer uses it)
    exchange = (volatile uchar *)malloc(4096);
    
    test_sophia_registration();
    test_sophia_startup();
    test_sophia_crash_and_recovery();
    
    free((void*)exchange);
    return 0;
}
