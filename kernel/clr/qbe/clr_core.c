/*
 * CLR Core Types Implementation (Object, String)
 * Native backends for System.Object and System.String InternalCalls.
 */

/* Manual Plan 9 Types */
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
typedef u32int Rune;
#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#define USED(x)                                                                \
  if (x) {                                                                     \
  }

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
typedef struct Fmt Fmt;

/* Structs provided by lib.h usually, but we need to ensure Qid/Dir/Waitmsg are
   defined if lib.h doesn't or if we need them before. lib.h DEFINES them. So we
   must include lib.h AFTER these typedefs but BEFORE usage. */
#include "../../port/lib.h"

#include "../../9front-pc64/mem.h"
#include "../../include/dat.h"
#include "../../include/fns.h"

#include "../clr-kernel/clr_pebble_integration.h"

/* External allocator */
extern void *pebble_alloc(ulong, void *);

/*
 * Helper: Extract C string from managed string object
 * Managed String Layout (assumed): [s32int length][u16int chars...]
 */
static void get_managed_string(clr_object_t *str_obj, char *buf, int max_len) {
  if (str_obj == nil || str_obj->data == nil) {
    if (max_len > 0)
      buf[0] = 0;
    return;
  }

  s32int *len_ptr = (s32int *)str_obj->data;
  s32int length = *len_ptr;

  u16int *chars = (u16int *)(len_ptr + 1);

  int i, out_idx = 0;
  for (i = 0; i < length && out_idx < max_len - 1; i++) {
    u16int c = chars[i];
    if (c < 128) {
      buf[out_idx++] = (char)c;
    } else {
      buf[out_idx++] = '?';
    }
  }
  buf[out_idx] = 0;
}

/*
 * System.Object.GetHashCode()
 * Returns the object pointer as the hash code (identity hash).
 */
int clr_object_gethashcode(clr_object_t *obj) {
  if (obj == nil)
    return 0;
  return (int)((uintptr)obj);
}

/*
 * System.Object.GetType()
 * Returns a Type object representing the runtime type of the object.
 * Uses the type_info field which contains the full type information.
 */
clr_object_t *clr_object_gettype(clr_object_t *obj) {
  if (obj == nil)
    return nil;

  /* Allocate a Type object - in a real implementation this would be cached */
  clr_object_t *type_obj = xalloc(sizeof(clr_object_t));
  if (type_obj == nil)
    return nil;

  /* Use the type enum value as the type representation */
  type_obj->type = obj->type;
  type_obj->type_info = obj->type_info;
  type_obj->data = obj->type_info; /* Point to the type info */
  type_obj->size = sizeof(clr_type_info_t);

  return type_obj;
}

/*
 * System.String.Internal_Concat2(string a, string b)
 */
clr_object_t *clr_string_concat2(clr_object_t *s1, clr_object_t *s2) {
  /* Handle null cases */
  if (s1 == nil || s1->data == nil)
    return s2;
  if (s2 == nil || s2->data == nil)
    return s1;

  /* Get lengths */
  s32int len1 = *(s32int *)s1->data;
  s32int len2 = *(s32int *)s2->data;
  s32int total_len = len1 + len2;

  /* Allocate new string: length field + chars */
  ulong size = sizeof(s32int) + total_len * sizeof(u16int);
  void *new_data = xalloc(size);
  if (new_data == nil)
    return s1; /* Allocation failed, return first string */

  /* Set length */
  *(s32int *)new_data = total_len;

  /* Copy chars from s1 */
  u16int *dst = (u16int *)((s32int *)new_data + 1);
  u16int *src1 = (u16int *)((s32int *)s1->data + 1);
  for (int i = 0; i < len1; i++)
    dst[i] = src1[i];

  /* Copy chars from s2 */
  u16int *src2 = (u16int *)((s32int *)s2->data + 1);
  for (int i = 0; i < len2; i++)
    dst[len1 + i] = src2[i];

  /* Allocate result object */
  clr_object_t *result = xalloc(sizeof(clr_object_t));
  if (result == nil) {
    xfree(new_data);
    return s1;
  }
  memset(result, 0, sizeof(clr_object_t));
  result->data = new_data;
  return result;
}

/*
 * System.String.Equals(string a, string b)
 */
int clr_string_equals(clr_object_t *s1, clr_object_t *s2) {
  if (s1 == s2)
    return 1;
  if (s1 == nil || s2 == nil)
    return 0;
  if (s1->data == nil || s2->data == nil)
    return 0;

  s32int len1 = *(s32int *)s1->data;
  s32int len2 = *(s32int *)s2->data;

  if (len1 != len2)
    return 0;

  u16int *c1 = (u16int *)((s32int *)s1->data + 1);
  u16int *c2 = (u16int *)((s32int *)s2->data + 1);

  for (int i = 0; i < len1; i++) {
    if (c1[i] != c2[i])
      return 0;
  }
  return 1;
}

/*
 * System.String.Internal_Concat3(string a, string b, string c)
 */
clr_object_t *clr_string_concat3(clr_object_t *s1, clr_object_t *s2,
                                 clr_object_t *s3) {
  /* Concatenate three strings by chaining concat2 */
  clr_object_t *temp = clr_string_concat2(s1, s2);
  if (temp == nil)
    return s3;
  return clr_string_concat2(temp, s3);
}

/*
 * System.String.Length (get)
 */
int clr_string_get_length(clr_object_t *str) {
  if (str == nil || str->data == nil)
    return 0;
  return *(s32int *)str->data;
}

/*
 * System.String.Chars (get_Chars)
 */
u16int clr_string_get_chars(clr_object_t *str, int index) {
  if (str == nil || str->data == nil)
    return 0;

  s32int len = *(s32int *)str->data;
  if (index < 0 || index >= len) {
    /* IndexOutOfRangeException - in kernel, return 0 */
    /* Full CLR would throw here */
    return 0;
  }

  u16int *chars = (u16int *)((s32int *)str->data + 1);
  return chars[index];
}

/*
 * System.Array.Length (get)
 */
int clr_array_get_length(clr_object_t *arr) {
  if (arr == nil || arr->data == nil)
    return 0;
  /* Assumed layout: [s32int length] ... */
  return *(s32int *)arr->data;
}

/*
 * System.Array.GetValue(int index)
 * NOTE: For MVP, assumes array of objects (clr_object_t*).
 * Value types (int[], etc.) would need boxing here.
 */
clr_object_t *clr_array_getvalue(clr_object_t *arr, int index) {
  if (arr == nil || arr->data == nil)
    return nil;

  s32int len = *(s32int *)arr->data;
  if (index < 0 || index >= len) {
    /* IndexOutOfRangeException - in kernel, return nil */
    /* Full CLR would throw here */
    return nil;
  }

  /* Layout: [length (4)] [pad (4)] [ptr0 (8)] [ptr1 (8)] ... */
  /* Alignment of pointers is 8 bytes */
  void **elements = (void **)((char *)arr->data + 8);
  return (clr_object_t *)elements[index];
}

/*
 * System.Array.SetValue(object value, int index)
 */
void clr_array_setvalue(clr_object_t *arr, clr_object_t *value, int index) {
  if (arr == nil || arr->data == nil)
    return;

  s32int len = *(s32int *)arr->data;
  if (index < 0 || index >= len)
    return;

  void **elements = (void **)((char *)arr->data + 8);

  /* Write barrier needed here! (clr_write_barrier(arr, value)) */
  /* For now, direct write. */
  elements[index] = value;
}

/*
 * System.Environment.TickCount (get)
 */
int clr_environment_tickcount(void) {
  /* Kernel µs() returns microseconds. Convert to ms. */
  return (int)(µs() / 1000);
}

/*
 * System.Environment.Exit(int)
 */
void clr_environment_exit(int code) {
  /* In kernel user process exit */
  exit(code);
  /* Unreachable */
}

/*
 * System.Environment.FailFast(string)
 */
void clr_environment_failfast(clr_object_t *msg) {
  char buf[256];
  get_managed_string(
      msg, buf,
      sizeof(buf)); /* Helper from clr_console? No, it's static there. */
  /* Reimplement get_managed_string locally or expose it? */
  /* For now, just panic with generic message to avoid duplicating helper
   * without refactoring. */
  panic("CLR FailFast triggered");
}

/*
 * System.Threading.Monitor.Enter(object)
 */
void clr_monitor_enter(clr_object_t *obj) {
  if (obj == nil)
    return;
  /* Use object's lock field for synchronization */
  lock(&obj->lock);
}

/*
 * System.Threading.Monitor.Exit(object)
 */
void clr_monitor_exit(clr_object_t *obj) {
  if (obj == nil)
    return;
  /* Use object's lock field for synchronization */
  unlock(&obj->lock);
}
