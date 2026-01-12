#include "lux.h"

extern void *memset(void *dst, int c, ulong n);
extern int atoi(const char *s);
extern unsigned long long strtoull(const char *s, char **endptr, int base);

/* Minimal SipHash-2-4 implementation */
#define ROTL(x, b) (u64int)(((x) << (b)) | ((x) >> (64 - (b))))

static void sipround(u64int *v0, u64int *v1, u64int *v2, u64int *v3) {
    *v0 += *v1; *v1 = ROTL(*v1, 13); *v1 ^= *v0; *v0 = ROTL(*v0, 32);
    *v2 += *v3; *v3 = ROTL(*v3, 16); *v3 ^= *v2; *v0 += *v3;
    *v3 = ROTL(*v3, 21); *v3 ^= *v0; *v2 += *v1; *v1 = ROTL(*v1, 17);
    *v1 ^= *v2; *v2 = ROTL(*v2, 32);
}

/* Optimized for 16-byte input (context + nonce) */
static u64int siphash24_16(u64int context, u64int nonce, const u8int seed[16]) {
    u64int k0 = ((u64int*)seed)[0];
    u64int k1 = ((u64int*)seed)[1];
    u64int v0 = 0x736f6d6570736575ULL ^ k0;
    u64int v1 = 0x646f72616e646f6dULL ^ k1;
    u64int v2 = 0x6c7967656e657261ULL ^ k0;
    u64int v3 = 0x7465646279746573ULL ^ k1;
    
    /* Block 1: Context */
    v3 ^= context;
    sipround(&v0, &v1, &v2, &v3); sipround(&v0, &v1, &v2, &v3);
    v0 ^= context;
    
    /* Block 2: Nonce */
    v3 ^= nonce;
    sipround(&v0, &v1, &v2, &v3); sipround(&v0, &v1, &v2, &v3);
    v0 ^= nonce;
    
    /* Finalize */
    u64int b = (u64int)16 << 56;
    v3 ^= b;
    sipround(&v0, &v1, &v2, &v3); sipround(&v0, &v1, &v2, &v3);
    v0 ^= b;
    v2 ^= 0xff;
    sipround(&v0, &v1, &v2, &v3); sipround(&v0, &v1, &v2, &v3);
    sipround(&v0, &v1, &v2, &v3); sipround(&v0, &v1, &v2, &v3);
    
    return v0 ^ v1 ^ v2 ^ v3;
}

int pow_solve(u64int context, int difficulty, u64int *nonce_out) {
    u8int seed[16];
    char buf[128];
    int fd;
    long n;
    
    /* Read seed from /dev/pebble/pow */
    fd = sys_open("/dev/pebble/pow", 0); /* OREAD */
    if(fd < 0) return -1;
    n = sys_read(fd, buf, sizeof(buf)-1);
    sys_close(fd);
    if(n <= 0) return -1;
    buf[n] = 0;
    
    /* Parse Hex Seed */
    /* Simple parse: 32 hex chars */
    char *p = buf;
    for(int i=0; i<16; i++) {
        char tmp[3] = {p[0], p[1], 0};
        seed[i] = (u8int)strtoull(tmp, 0, 16);
        p += 2;
    }
    
    u64int nonce = 0;
    while(1) {
        u64int h = siphash24_16(context, nonce, seed);
        
        int zeros = 0;
        if (h == 0) zeros = 64;
        else {
            u64int mask = 1ULL << 63;
            while((h & mask) == 0) { zeros++; mask >>= 1; }
        }
        
        if (zeros >= difficulty) {
            *nonce_out = nonce;
            return 0;
        }
        nonce++;
        /* Safety cap */
        if (nonce > 10000000) return -1;
    }
}

int msgord_submit(char *path, MsgOrdFcall *t) {
    /* Wrapper for future use */
    return -1;
}
