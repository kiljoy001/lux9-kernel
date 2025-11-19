/* Minimal init - test syscall argument passing */

/* Plan 9 syscall numbers */
#define PWRITE  51
#define EXITS   8

/* External syscall wrapper from syscall_amd64.S */
extern long __syscall(long syscall_num, ...);

/* Simple string length */
static int
strlen(const char *s)
{
    int n = 0;
    while(*s++) n++;
    return n;
}

/* Entry point - called by _start */
void
_main(void)
{
    const char msg[] = "=== Minimal init starting ===\n";
    const char msg2[] = "About to call PWRITE...\n";
    long ret;

    /* First write a shorter message */
    __syscall(PWRITE, 1, msg2, 24, 0LL);

    /* PWRITE(fd, buf, len, offset) - write to stdout (fd=1) */
    ret = __syscall(PWRITE, 1, msg, strlen(msg), 0LL);

    /* Print result */
    if(ret > 0) {
        const char ok[] = "PWRITE succeeded!\n";
        __syscall(PWRITE, 1, ok, strlen(ok), 0LL);
    } else {
        const char err[] = "PWRITE failed!\n";
        __syscall(PWRITE, 1, err, strlen(err), 0LL);
    }

    /* Loop forever so we can see output */
    const char done[] = "Init complete, looping...\n";
    __syscall(PWRITE, 1, done, strlen(done), 0LL);

    for(;;);

    /* EXITS(status) - exit */
    __syscall(EXITS, 0);
}

/* Minimal startup code */
__asm__(
    ".text\n"
    ".globl _start\n"
    "_start:\n"
    "    xorq %rbp, %rbp\n"      /* Clear frame pointer */
    "    andq $-16, %rsp\n"       /* Align stack to 16 bytes */
    "    call _main\n"            /* Call main */
    "    movq $8, %rbp\n"         /* EXITS syscall */
    "    xorq %rsp, %rsp\n"       /* No arguments */
    "    syscall\n"               /* Exit */
    "    jmp .\n"                 /* Should never reach here */
);
