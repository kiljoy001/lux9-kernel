/*
 * CLR Console Implementation (System.Console)
 * Maps managed strings to /dev/console output.
 */

/* Manual Plan 9 Types (Nuclear Option to avoid header hell) */
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

/* Use kernel print function */
extern int print(char *, ...);

/*
 * Helper: Extract C string from managed string object
 * Managed String Layout (assumed): [s32int length][u16int chars...]
 */
static void get_managed_string(clr_object_t *str_obj, char *buf, int max_len) {
  /* MVP: str_obj IS the data pointer (s32 len + u16 chars) */
  /* Full impl would use str_obj->data */
  if (str_obj == nil) {
    if (max_len > 0)
      buf[0] = 0;
    return;
  }

  /*
   * Read length (first 4 bytes).
   * Note: This assumes specific layout. If Fruity IR changes layout, this
   * breaks.
   */
  s32int *len_ptr = (s32int *)str_obj;
  s32int length = *len_ptr;

  /* Pointer to UTF-16 chars */
  u16int *chars = (u16int *)(len_ptr + 1);

  int i, out_idx = 0;
  for (i = 0; i < length && out_idx < max_len - 1; i++) {
    u16int c = chars[i];
    /* Simple ASCII downcast for now - full UTF-8 conversion needed later */
    if (c < 128) {
      buf[out_idx++] = (char)c;
    } else {
      buf[out_idx++] = '?';
    }
  }
  buf[out_idx] = 0;
}

/*
 * InternalCall: System.Console.Internal_Write(string)
 */
void clr_console_write(clr_object_t *str_obj) {
  char buf[1024];
  get_managed_string(str_obj, buf, sizeof(buf));
  print("%s", buf);
}

/*
 * InternalCall: System.Console.Internal_WriteLine(string)
 */
void clr_console_writeline(clr_object_t *str_obj) {
  char buf[1024];
  print("CLR_DEBUG: clr_console_writeline CALLED str_obj=%p\n", str_obj);
  if (str_obj == nil) {
    print("CLR_DEBUG: str_obj is NULL\n");
    print("\n"); /* Just print newline */
    return;
  }
  print("CLR_DEBUG: str_obj->data=%p\n", str_obj->data);
  get_managed_string(str_obj, buf, sizeof(buf));
  print("CLR_DEBUG: extracted string='%s'\n", buf);
  print("%s\n", buf);
}
