/*
 * clr_bcl_helpers.c - Kernel-backed BCL helper functions
 *
 * These C functions are called via InternalCall from BCL C# code
 * to provide high-performance runtime support.
 */

/* Manual Plan 9 Types (avoiding header conflicts) */
#define _U_H_
#define nil ((void *)0)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
#define nelem(x) (sizeof(x) / sizeof((x)[0]))

/* Minimal function declarations */
extern void *memset(void *s, int c, ulong n);
extern void *memmove(void *dst, void *src, ulong n);
extern uvlong fastticks(uvlong *hz);
extern uvlong fastticks2ns(uvlong ticks);

/*
 * DateTime helpers
 *
 * .NET DateTime uses ticks = 100-nanosecond intervals since 0001-01-01
 * Kernel fastticks gives us high-resolution monotonic time
 */

/* Ticks per second (10 million = 100ns intervals) */
#define TICKS_PER_SECOND 10000000LL
#define TICKS_PER_MILLISECOND 10000LL

/* Unix epoch offset: ticks from 0001-01-01 to 1970-01-01 */
#define UNIX_EPOCH_TICKS 621355968000000000LL

/*
 * clr_datetime_now - Get current DateTime as ticks
 * Returns: ticks since 0001-01-01 in .NET format
 */
vlong clr_datetime_now(void) {
  uvlong hz, ticks, ns;
  vlong dotnet_ticks;

  hz = 0;
  ticks = fastticks(&hz);

  if (hz == 0)
    return UNIX_EPOCH_TICKS; /* Fallback if clock not ready */

  /* Convert fastticks to nanoseconds, then to .NET ticks (100ns) */
  ns = fastticks2ns(ticks);
  dotnet_ticks = (vlong)(ns / 100);

  /* Add Unix epoch offset (assume kernel time is Unix-based) */
  return UNIX_EPOCH_TICKS + dotnet_ticks;
}

/*
 * clr_datetime_tickcount - Get milliseconds since boot (Environment.TickCount)
 */
int clr_datetime_tickcount(void) {
  uvlong ns = fastticks2ns(fastticks(nil));
  return (int)(ns / 1000000); /* Convert ns to ms */
}

/*
 * BigInteger helpers
 *
 * Provide fast multiply/divide for arbitrary precision integers.
 * These avoid expensive managed array operations.
 */

/*
 * clr_bigint_add - Add two magnitude arrays
 * left/right: uint32 arrays, leftLen/rightLen: lengths
 * result: output array (must be pre-allocated with max(leftLen,rightLen)+1
 * elements) Returns: actual result length
 */
int clr_bigint_add(u32int *left, int leftLen, u32int *right, int rightLen,
                   u32int *result) {
  u32int *longer, *shorter;
  int longLen, shortLen;
  int i;
  uvlong carry;

  if (leftLen >= rightLen) {
    longer = left;
    longLen = leftLen;
    shorter = right;
    shortLen = rightLen;
  } else {
    longer = right;
    longLen = rightLen;
    shorter = left;
    shortLen = leftLen;
  }

  carry = 0;

  for (i = 0; i < shortLen; i++) {
    uvlong sum = (uvlong)longer[i] + shorter[i] + carry;
    result[i] = (u32int)sum;
    carry = sum >> 32;
  }

  for (; i < longLen; i++) {
    uvlong sum = (uvlong)longer[i] + carry;
    result[i] = (u32int)sum;
    carry = sum >> 32;
  }

  if (carry) {
    result[i++] = (u32int)carry;
  }

  return i;
}

/*
 * clr_bigint_sub - Subtract magnitude arrays (left >= right assumed)
 * Returns: actual result length
 */
int clr_bigint_sub(u32int *left, int leftLen, u32int *right, int rightLen,
                   u32int *result) {
  vlong borrow = 0;
  int i;

  for (i = 0; i < rightLen; i++) {
    vlong diff = (vlong)left[i] - right[i] - borrow;
    if (diff < 0) {
      diff += 0x100000000LL;
      borrow = 1;
    } else {
      borrow = 0;
    }
    result[i] = (u32int)diff;
  }

  for (; i < leftLen; i++) {
    vlong diff = (vlong)left[i] - borrow;
    if (diff < 0) {
      diff += 0x100000000LL;
      borrow = 1;
    } else {
      borrow = 0;
    }
    result[i] = (u32int)diff;
  }

  /* Trim leading zeros */
  while (i > 0 && result[i - 1] == 0)
    i--;
  return i;
}

/*
 * clr_bigint_mul - Multiply magnitude arrays
 * result must be pre-allocated with leftLen + rightLen elements
 * Returns: actual result length
 */
int clr_bigint_mul(u32int *left, int leftLen, u32int *right, int rightLen,
                   u32int *result) {
  int resultLen = leftLen + rightLen;
  int i, j;

  /* Clear result */
  memset(result, 0, resultLen * sizeof(u32int));

  for (i = 0; i < leftLen; i++) {
    uvlong carry = 0;
    for (j = 0; j < rightLen; j++) {
      uvlong product = (uvlong)left[i] * right[j] + result[i + j] + carry;
      result[i + j] = (u32int)product;
      carry = product >> 32;
    }
    result[i + rightLen] = (u32int)carry;
  }

  /* Trim leading zeros */
  while (resultLen > 0 && result[resultLen - 1] == 0)
    resultLen--;
  return resultLen > 0 ? resultLen : 1;
}

/*
 * clr_bigint_div_small - Divide by single-word divisor (common case)
 * For arbitrary divisors, caller should fall back to managed code
 */
int clr_bigint_div_small(u32int *dividend, int dividendLen, u32int divisor,
                         u32int *quotient, u32int *remainder) {
  uvlong rem;
  int i;

  if (divisor == 0)
    return -1; /* Division by zero */

  rem = 0;

  for (i = dividendLen - 1; i >= 0; i--) {
    uvlong cur = (rem << 32) | dividend[i];
    quotient[i] = (u32int)(cur / divisor);
    rem = cur % divisor;
  }

  *remainder = (u32int)rem;

  /* Trim leading zeros */
  while (dividendLen > 0 && quotient[dividendLen - 1] == 0)
    dividendLen--;
  return dividendLen > 0 ? dividendLen : 1;
}

/*
 * Span/Memory helpers
 */

/*
 * clr_span_clear - Clear a span (memset to 0)
 */
void clr_span_clear(void *ptr, int elementSize, int count) {
  if (ptr != nil && count > 0) {
    memset(ptr, 0, (ulong)elementSize * count);
  }
}

/*
 * clr_span_copy - Copy span contents
 */
void clr_span_copy(void *dst, void *src, int elementSize, int count) {
  if (dst != nil && src != nil && count > 0) {
    memmove(dst, src, (ulong)elementSize * count);
  }
}

/*
 * Decimal helpers (128-bit arithmetic)
 */

/*
 * clr_decimal_add96 - Add two 96-bit integers (as 3 x uint32)
 * Returns carry (0 or 1)
 */
int clr_decimal_add96(u32int *a, u32int *b, u32int *result) {
  uvlong sum = (uvlong)a[0] + b[0];
  result[0] = (u32int)sum;

  sum = (uvlong)a[1] + b[1] + (sum >> 32);
  result[1] = (u32int)sum;

  sum = (uvlong)a[2] + b[2] + (sum >> 32);
  result[2] = (u32int)sum;

  return (int)(sum >> 32);
}

/*
 * clr_decimal_mul32 - Multiply 96-bit by 32-bit (for scaling)
 * Returns overflow into 4th word
 */
u32int clr_decimal_mul32(u32int *a, u32int multiplier, u32int *result) {
  uvlong product = (uvlong)a[0] * multiplier;
  result[0] = (u32int)product;

  product = (uvlong)a[1] * multiplier + (product >> 32);
  result[1] = (u32int)product;

  product = (uvlong)a[2] * multiplier + (product >> 32);
  result[2] = (u32int)product;

  return (u32int)(product >> 32);
}
