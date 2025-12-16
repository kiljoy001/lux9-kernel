/* Comprehensive syscall test for Lux9 kernel */

/* Plan 9 syscall numbers */
#define SYSR1       0
#define BIND        2
#define CHDIR       3
#define CLOSE       4
#define DUP         5
#define ALARM       6
#define EXEC        7
#define EXITS       8
#define FAUTH       10
#define SEGBRK      12
#define OPEN        14
#define OSEEK       16
#define SLEEP       17
#define STAT        18
#define RFORK       19
#define WRITE       20
#define PIPE        21
#define CREATE      22
#define FD2PATH     23
#define BRK         24
#define REMOVE      25
#define NOTIFY      28
#define NOTED       29
#define SEGATTACH   30
#define SEGDETACH   31
#define SEGFREE     32
#define SEGFLUSH    33
#define RENDEZVOUS  34
#define UNMOUNT     35
#define FWSTAT      36
#define FSTAT       37
#define AWAIT       39
#define PREAD       50
#define PWRITE      51
#define SEEK        52

/* Open modes */
#define OREAD   0
#define OWRITE  1
#define ORDWR   2
#define OEXEC   3
#define OTRUNC  16
#define OCEXEC  32
#define ORCLOSE 64
#define OEXCL   0x1000

/* Syscall wrapper - Plan 9 convention: rbp=syscall#, rsp=args */
long __syscall(long syscall_num, ...);
__asm__(
    ".globl __syscall\n"
    "__syscall:\n"
    "    push %rbp\n"
    "    push %rbx\n"
    "    push %r12\n"
    "    push %r13\n"
    "    sub $0x30, %rsp\n"
    "    mov %rsp, %r12\n"
    "    mov %rsi, (%r12)\n"       /* arg1 */
    "    mov %rdx, 0x8(%r12)\n"    /* arg2 */
    "    mov %rcx, 0x10(%r12)\n"   /* arg3 */
    "    mov %r8, 0x18(%r12)\n"    /* arg4 */
    "    mov %r9, 0x20(%r12)\n"    /* arg5 */
    "    mov %rdi, %rbx\n"         /* syscall number */
    "    mov %rsp, %r13\n"
    "    mov %rbx, %rbp\n"         /* rbp = syscall number */
    "    syscall\n"
    "    mov %rsp, %r13\n"
    "    add $0x30, %r13\n"
    "    mov %r13, %rsp\n"
    "    pop %r13\n"
    "    pop %r12\n"
    "    pop %rbx\n"
    "    pop %rbp\n"
    "    ret\n"
);

/* Simple string functions */
static int
strlen(const char *s)
{
    int n = 0;
    while(*s++) n++;
    return n;
}

static void
strcpy(char *dst, const char *src)
{
    while((*dst++ = *src++))
        ;
}

static int
strcmp(const char *s1, const char *s2)
{
    while(*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/* Output helpers */
static void
print(const char *s)
{
    __syscall(PWRITE, 1, s, strlen(s), 0LL);
}

static void
print_num(long n)
{
    char buf[32];
    char *p = buf + sizeof(buf) - 1;
    int neg = 0;

    *p = 0;
    if(n < 0) {
        neg = 1;
        n = -n;
    }
    if(n == 0) {
        *--p = '0';
    } else {
        while(n > 0) {
            *--p = '0' + (n % 10);
            n /= 10;
        }
    }
    if(neg)
        *--p = '-';
    print(p);
}

static void
print_hex(unsigned long n)
{
    char buf[32];
    char *p = buf + sizeof(buf) - 1;
    static char hex[] = "0123456789abcdef";

    *p = 0;
    if(n == 0) {
        *--p = '0';
    } else {
        while(n > 0) {
            *--p = hex[n & 0xf];
            n >>= 4;
        }
    }
    *--p = 'x';
    *--p = '0';
    print(p);
}

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

static void
test_pass(const char *name)
{
    print("[PASS] ");
    print(name);
    print("\n");
    tests_passed++;
}

static void
test_fail(const char *name, long expected, long got)
{
    print("[FAIL] ");
    print(name);
    print(" - expected ");
    print_num(expected);
    print(", got ");
    print_num(got);
    print("\n");
    tests_failed++;
}

static void
test_fail_msg(const char *name, const char *msg)
{
    print("[FAIL] ");
    print(name);
    print(" - ");
    print(msg);
    print("\n");
    tests_failed++;
}

/* Individual syscall tests */

static void
test_pwrite(void)
{
    const char *msg = "PWRITE test data\n";
    long ret = __syscall(PWRITE, 1, msg, strlen(msg), 0LL);
    if(ret == strlen(msg))
        test_pass("PWRITE to stdout");
    else
        test_fail("PWRITE to stdout", strlen(msg), ret);
}

static void
test_write(void)
{
    const char *msg = "WRITE test data\n";
    long ret = __syscall(WRITE, 1, msg, strlen(msg));
    if(ret == strlen(msg))
        test_pass("WRITE to stdout");
    else
        test_fail("WRITE to stdout", strlen(msg), ret);
}

static void
test_open_close(void)
{
    /* Try to open console */
    long fd = __syscall(OPEN, "#c/cons", OREAD);
    if(fd >= 0) {
        test_pass("OPEN #c/cons");
        long ret = __syscall(CLOSE, fd);
        if(ret == 0)
            test_pass("CLOSE");
        else
            test_fail("CLOSE", 0, ret);
    } else {
        test_fail("OPEN #c/cons", 0, fd);
    }
}

static void
test_dup(void)
{
    /* Dup stdout */
    long fd = __syscall(DUP, 1, -1);
    if(fd >= 0) {
        test_pass("DUP stdout");
        /* Write to duped fd */
        const char *msg = "DUP write test\n";
        long ret = __syscall(PWRITE, fd, msg, strlen(msg), 0LL);
        if(ret == strlen(msg))
            test_pass("PWRITE to DUPed fd");
        else
            test_fail("PWRITE to DUPed fd", strlen(msg), ret);
        __syscall(CLOSE, fd);
    } else {
        test_fail("DUP stdout", 0, fd);
    }
}

static void
test_fd2path(void)
{
    char buf[256];
    long ret = __syscall(FD2PATH, 1, buf, sizeof(buf));
    if(ret == 0) {
        test_pass("FD2PATH on stdout");
        print("  stdout path: ");
        print(buf);
        print("\n");
    } else {
        test_fail("FD2PATH on stdout", 0, ret);
    }
}

static void
test_stat(void)
{
    /* Stat buffer - Plan 9 Dir structure is complex, just check return */
    char buf[512];
    long ret = __syscall(STAT, "#c/cons", buf, sizeof(buf));
    if(ret > 0) {
        test_pass("STAT #c/cons");
        print("  stat returned ");
        print_num(ret);
        print(" bytes\n");
    } else {
        test_fail("STAT #c/cons", 1, ret);
    }
}

static void
test_fstat(void)
{
    char buf[512];
    long ret = __syscall(FSTAT, 1, buf, sizeof(buf));
    if(ret > 0) {
        test_pass("FSTAT on stdout");
        print("  fstat returned ");
        print_num(ret);
        print(" bytes\n");
    } else {
        test_fail("FSTAT on stdout", 1, ret);
    }
}

static void
test_brk(void)
{
    /* Get current break */
    long brk = __syscall(BRK, 0);
    if(brk > 0) {
        test_pass("BRK get current");
        print("  current brk: ");
        print_hex(brk);
        print("\n");

        /* Try to extend */
        long newbrk = __syscall(BRK, brk + 4096);
        if(newbrk >= brk + 4096) {
            test_pass("BRK extend");
            print("  new brk: ");
            print_hex(newbrk);
            print("\n");
        } else {
            test_fail("BRK extend", brk + 4096, newbrk);
        }
    } else {
        test_fail("BRK get current", 1, brk);
    }
}

static void
test_seek(void)
{
    /* Open a file we can seek */
    long fd = __syscall(OPEN, "#c/cons", OREAD);
    if(fd >= 0) {
        /* Seek is a bit tricky - console may not support it */
        long ret = __syscall(SEEK, fd, 0LL, 0);
        if(ret >= 0) {
            test_pass("SEEK on console");
        } else {
            /* Expected to fail on console */
            print("[INFO] SEEK on console returned ");
            print_num(ret);
            print(" (expected for stream)\n");
        }
        __syscall(CLOSE, fd);
    }
}

static void
test_pipe(void)
{
    long fds[2];
    long ret = __syscall(PIPE, fds);
    if(ret == 0) {
        test_pass("PIPE create");
        print("  pipe fds: ");
        print_num(fds[0]);
        print(", ");
        print_num(fds[1]);
        print("\n");

        /* Write to pipe */
        const char *msg = "pipe test";
        ret = __syscall(WRITE, fds[1], msg, strlen(msg));
        if(ret == strlen(msg)) {
            test_pass("WRITE to pipe");

            /* Read from pipe */
            char buf[64];
            ret = __syscall(PREAD, fds[0], buf, sizeof(buf)-1, 0LL);
            if(ret > 0) {
                buf[ret] = 0;
                test_pass("PREAD from pipe");
                print("  read: '");
                print(buf);
                print("'\n");
            } else {
                test_fail("PREAD from pipe", 1, ret);
            }
        } else {
            test_fail("WRITE to pipe", strlen(msg), ret);
        }

        __syscall(CLOSE, fds[0]);
        __syscall(CLOSE, fds[1]);
    } else {
        test_fail("PIPE create", 0, ret);
    }
}

static void
test_bind(void)
{
    /* Try a simple bind - this tests the namespace */
    long ret = __syscall(BIND, "#c", "/dev", 0);
    if(ret == 0) {
        test_pass("BIND #c to /dev");
    } else {
        /* May fail if already bound */
        print("[INFO] BIND #c /dev returned ");
        print_num(ret);
        print("\n");
    }
}

static void
test_chdir(void)
{
    long ret = __syscall(CHDIR, "/");
    if(ret == 0) {
        test_pass("CHDIR /");
    } else {
        test_fail("CHDIR /", 0, ret);
    }
}

static void
test_sleep(void)
{
    print("Testing SLEEP 100ms...\n");
    long ret = __syscall(SLEEP, 100);
    if(ret >= 0) {
        test_pass("SLEEP 100ms");
    } else {
        test_fail("SLEEP 100ms", 0, ret);
    }
}

static void
test_alarm(void)
{
    /* Set alarm for 1000ms */
    long ret = __syscall(ALARM, 1000);
    if(ret >= 0) {
        test_pass("ALARM set");
        print("  previous alarm: ");
        print_num(ret);
        print("ms\n");

        /* Cancel it */
        ret = __syscall(ALARM, 0);
        if(ret >= 0)
            test_pass("ALARM cancel");
        else
            test_fail("ALARM cancel", 0, ret);
    } else {
        test_fail("ALARM set", 0, ret);
    }
}

/* Main test runner */
void
_main(void)
{
    print("\n");
    print("==========================================\n");
    print("   Lux9 Syscall Test Suite\n");
    print("==========================================\n\n");

    /* Basic I/O */
    test_pwrite();
    test_write();

    /* File operations */
    test_open_close();
    test_dup();
    test_fd2path();
    test_stat();
    test_fstat();

    /* Memory */
    test_brk();

    /* Pipes */
    test_pipe();

    /* Namespace */
    test_bind();
    test_chdir();

    /* Time */
    test_sleep();
    test_alarm();

    /* Summary */
    print("\n==========================================\n");
    print("   Test Summary\n");
    print("==========================================\n");
    print("Passed: ");
    print_num(tests_passed);
    print("\nFailed: ");
    print_num(tests_failed);
    print("\nTotal:  ");
    print_num(tests_passed + tests_failed);
    print("\n==========================================\n\n");

    if(tests_failed == 0) {
        print("All tests passed!\n");
    } else {
        print("Some tests failed.\n");
    }

    print("\nTest complete, spinning...\n");
    for(;;);

    __syscall(EXITS, 0);
}

/* Startup code */
__asm__(
    ".text\n"
    ".globl _start\n"
    "_start:\n"
    "    xorq %rbp, %rbp\n"
    "    andq $-16, %rsp\n"
    "    call _main\n"
    "    movq $8, %rbp\n"
    "    xorq %rsp, %rsp\n"
    "    syscall\n"
    "    jmp .\n"
);
