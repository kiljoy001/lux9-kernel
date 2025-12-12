#include "dat.h"
#include "fns.h"
#include "u.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Global State */
Mach *m;
Proc *up;
Conf conf;
Mach mock_mach;
Proc mock_proc;
int xinit_done = 1;
uintptr saved_limine_hhdm_offset = 0;

void genrandom(uchar *p, int n) {
  for (int i = 0; i < n; i++)
    p[i] = rand();
}

/* Error Handling (simulated with setjmp/longjmp) */
static jmp_buf error_buf;
static int error_active = 0;

void error(char *s) {
  printf("ERROR RAISED: %s\n", s);
  if (up)
    strncpy(up->errstr, s, ERRMAX - 1);
  if (error_active)
    longjmp(error_buf, 1);
  else {
    printf("Fatal error without handler: %s\n", s);
    exit(1);
  }
}

int waserror(void) {
  error_active = 1;
  return setjmp(error_buf);
}

void poperror(void) { error_active = 0; }

void nexterror(void) { error(up->errstr); }

/* Memory Allocation */
void *malloc(ulong size) { return mallocz(size, 0); }

void *mallocz(ulong size, int clear) {
  void *p = calloc(1, size); // ASan will track this
  if (p == NULL && size > 0)
    return NULL;
  return p;
}

/* free is provided by libc */

void *xalloc(ulong size) {
  // Simulate physical memory with malloc
  return mallocz(size, 1);
}

void *xallocz(ulong size, int clear) { return mallocz(size, clear); }

void xfree(void *ptr) { free(ptr); }
void *bootstrap_alloc(ulong size) { return mallocz(size, 1); }

/* Locks */
void lock(Lock *l) {}
void unlock(Lock *l) {}
void ilock(Lock *l) {}
void iunlock(Lock *l) {}
int canlock(Lock *l) { return 1; }

/* IO */
void print(char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  vprintf(fmt, arg);
  va_end(arg);
}

void panic(char *fmt, ...) {
  printf("\nPANIC: ");
  va_list arg;
  va_start(arg, fmt);
  vprintf(fmt, arg);
  va_end(arg);
  printf("\n");
  abort(); // Trigger core dump / ASan report
}

ulong getcallerpc(void *arg) { return 0; }

/* Crypto Mocks */
int tpm_get_random(u8int *buffer, int len) {
  for (int i = 0; i < len; i++)
    buffer[i] = rand();
  return len;
}
u64int rdrand_u64(void) { return (u64int)rand() << 32 | rand(); }
int crypto_hw_rdrand_available(void) { return 1; }
// Stub for TPM HMAC
int crypto_tpm_hmac_sha256(u8int *out, const u8int *data, ulong len) {
  // In stub, just do a normal software HMAC with a dummy key
  u8int dummy_key[32] = {0xAA};
  // Prototype needed to avoid implicit declaration warning
  void crypto_hmac_sha256(u8int * out, u8int * key, ulong klen, u8int * data,
                          ulong dlen);
  crypto_hmac_sha256(out, dummy_key, 32, (u8int *)data, len);
  return 0; // Success
}

int crypto_tpm_key_init(void) { return 0; }
int crypto_tpm_rotate_hmac_key(void) { return 0; }
int crypto_hw_sha_available(void) { return 0; }
u64int chacha20_csprng_u64(void) { return rdrand_u64(); }

/* Crypto Implementation Mocks (Simple but unique) */
void crypto_sha256(u8int *out, u8int *in, ulong len) {
  // Simple hash: XOR all input bytes and spread across output
  memset(out, 0, 32);
  for (ulong i = 0; i < len; i++) {
    out[i % 32] ^= in[i];
    out[(i + 17) % 32] ^= (in[i] << 4) | (in[i] >> 4);
  }
  // Add length for uniqueness
  out[31] ^= (u8int)(len & 0xFF);
  out[30] ^= (u8int)((len >> 8) & 0xFF);
}
void crypto_hmac_sha256(u8int *out, u8int *key, ulong klen, u8int *data,
                        ulong dlen) {
  memset(out, 0, 32);
  for (ulong i = 0; i < klen; i++)
    out[i % 32] ^= key[i];
  for (ulong i = 0; i < dlen; i++)
    out[(i + 16) % 32] ^= data[i];
}

/* SipHash Mock */
/* Must match siphash.h signatures if linked, but here we compile stub logic or
   use copy? We copied siphash.h. But we construct stub implementations here. If
   siphash.h declares functions, we must match.
*/
void siphash_init(void *k, void *k128) {}
u64int siphash(void *in, int len, void *k) { return 0; }
u32int hsiphash(void *in, int len, void *k) { return 0; }

/* Time */
uvlong todget(void *a, void *b) { return 0; }

/* Init Globals */
void lockdag_init(void) {}

void stubs_init(void) {
  m = &mock_mach;
  up = &mock_proc;
  up->pid = 1;
  strcpy(up->text, "test_init");
}
