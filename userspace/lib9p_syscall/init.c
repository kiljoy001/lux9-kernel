/* Minimal init using syscall message passing
 * Demonstrates proper Tsyscall/SYS_WRITE via exchange page
 */

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00
#define Tsyscall 130
#define SYS_WRITE 4

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long long uvlong;

/* P9 Control structure */
struct P9Control {
    uint doorbell;
    uint status;
    uint req_head;
    uint req_tail;
    uint rep_head;
    uint rep_tail;
};

/* Simple memory operations */
static void *memcpy(void *dst, const void *src, unsigned long n) {
    uchar *d = dst;
    const uchar *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

static void *memset(void *dst, int c, unsigned long n) {
    uchar *d = dst;
    while (n--) *d++ = (uchar)c;
    return dst;
}

/* Write little-endian integers */
static void put_u32(uchar *p, uint val) {
    p[0] = val;
    p[1] = val >> 8;
    p[2] = val >> 16;
    p[3] = val >> 24;
}

static void put_u16(uchar *p, unsigned short val) {
    p[0] = val;
    p[1] = val >> 8;
}

static void put_u64(uchar *p, uvlong val) {
    p[0] = val;
    p[1] = val >> 8;
    p[2] = val >> 16;
    p[3] = val >> 24;
    p[4] = val >> 32;
    p[5] = val >> 40;
    p[6] = val >> 48;
    p[7] = val >> 56;
}

void main(void) {
    volatile uchar *exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
    volatile struct P9Control *ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);
    const char *msg = "Hello from userspace!\n";
    unsigned int msg_len = 22;
    unsigned int counter = 0;

    while(1) {
        /* Build Tsyscall message: [size:4] [type:1] [tag:2] [scallnr:4] [scount:4] [sdata:n]
         * sdata for SYS_WRITE: [fid:4] [offset:8] [count:4] [data...]
         */
        uchar *req = (uchar *)exchange;
        unsigned int sdata_size = 4 + 8 + 4 + msg_len;  /* fid + offset + count + data */
        unsigned int size = 4 + 1 + 2 + 4 + 4 + sdata_size;  /* full message size */
        unsigned int pos = 0;

        /* Clear request buffer */
        memset(req, 0, 256);

        /* Write Tsyscall header */
        put_u32(req + pos, size); pos += 4;           /* size */
        req[pos++] = Tsyscall;                        /* type = 130 */
        put_u16(req + pos, 1); pos += 2;              /* tag */
        put_u32(req + pos, SYS_WRITE); pos += 4;      /* scallnr = 4 */
        put_u32(req + pos, sdata_size); pos += 4;     /* scount */

        /* Write SYS_WRITE payload (sdata) */
        put_u32(req + pos, 1); pos += 4;              /* fid = 1 (stdout fd) */
        put_u64(req + pos, counter); pos += 8;        /* offset (use counter to show repeated messages) */
        put_u32(req + pos, msg_len); pos += 4;        /* count */
        memcpy(req + pos, msg, msg_len);              /* data */

        /* Ring the doorbell */
        ctl->doorbell = 1;

        /* Issue syscall (doorbell trigger) */
        __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

        /* Wait a bit to make output readable */
        for (volatile int i = 0; i < 1000000; i++)
            ;

        counter++;
    }
}
