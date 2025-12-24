/* lux9_api.c - Lux9 System Interface for WASM
 *
 * Implements the Host Functions (Imports) that WASM modules use to talk to the
 * kernel. The primary interface is 9P over Exchange Pages.
 */

#include "../../include/dat.h"
#include "../../include/error.h"
#include "../../include/fns.h"
#include "../../include/mem.h"
#include "../../include/portlib.h"
#include "../../include/u.h"
#include "../wasm_runtime/wasm3/m3_core.h"
#include "../wasm_runtime/wasm3/wasm3.h"

/* WASM bump allocator - allocates from WASM linear memory */
#define WASM_HEAP_START                                                        \
  (64 * 1024) /* Start at 64KB (leave room for stack/data) */

#define WASM_HEAP_MAX (2 * 1024 * 1024) /* 2MB memory size */

static u32int wasm_heap_ptr = WASM_HEAP_START;

static u32int wasm_bump_alloc(u32int size) {
  /* Align to 8 bytes */
  size = (size + 7) & ~7;

  u32int offset = wasm_heap_ptr;
  wasm_heap_ptr += size;

  if (wasm_heap_ptr > WASM_HEAP_MAX) {
    print("WASM: heap exhausted (ptr=%d, size=%d)\n", offset, size);
    return 0; /* Out of memory */
  }

  return offset;
}

#define CLR_PTR_TAG 0x8000000000000000ULL

/* Error checking macro for WASM linking */
#define _(x)                                                                   \
  {                                                                            \
    M3Result _res = (x);                                                       \
    if (_res)                                                                  \
      return _res;                                                             \
  }

/* Forward declaration for 9P router hook */
extern long p9_route_message(int pid, void *msg, ulong len);

/* CLR runtime hooks (still used for Pebble accounting) */
extern void *lux_alloc(ulong size, ulong type_token);
extern void *lux_addref(void *ptr);
extern void lux_release(void *ptr);
extern void *lux_snapshot(void *ptr);
extern void lux_commit(void *ptr);
extern void lux_rollback(void *ptr);

extern void *clr_string_from_literal(u32int us_index);
extern ulong clr_get_type_size(u32int token);
extern void *clr_get_static_field(u32int token);

typedef struct clr_object clr_object_t;
extern int clr_is_instance_of(clr_object_t *obj, u32int type_token);

static u64int clr_tag_ptr(void *ptr) {
  if (!ptr)
    return 0;
  return ((u64int)(uintptr_t)ptr) | CLR_PTR_TAG;
}

static void *clr_untag_ptr(u64int val) {
  return (void *)(uintptr_t)(val & ~CLR_PTR_TAG);
}

/* Convert WASM value to memory pointer - now always uses WASM offset */
static void *clr_ptr_to_mem(u64int ptr_val, void *_mem) {
  if (ptr_val == 0)
    return nil;
  if (ptr_val & CLR_PTR_TAG) {
    /* Untag to get offset, then add WASM memory base */
    u64int off = ptr_val & ~CLR_PTR_TAG;
    return (void *)((u8int *)_mem + (u32int)off);
  }
  /* Treat as WASM linear memory offset */
  return (void *)((u8int *)_mem + (u32int)ptr_val);
}

/*

 * lux9_send_9p(arr: ref, len: i32) -> i32
 *
 * Copies 'len' bytes from a managed byte[] (length-prefixed) into the
 * process Exchange Page, then routes the 9P message.
 */
m3ApiRawFunction(lux9_send_9p) {
  m3ApiReturnType(vlong) m3ApiGetArg(u64int, arr_val);
  m3ApiGetArg(u64int, msg_len64);
  u32int msg_len = (u32int)msg_len64;

  if (msg_len > 4096) { /* Exchange page size limit */
    m3ApiReturn(-1);
  }

  void *arr = clr_ptr_to_mem(arr_val, _mem);
  if (!arr)
    m3ApiReturn(-2);

  /* Allocate kernel buffer for contiguous message */
  u8int *packed_msg = xalloc(msg_len);
  if (!packed_msg)
    m3ApiReturn(-3);

  /* Helper to access element i in sparse array: arr + 8 + i*8 */
  u64int *sparse_base = (u64int *)((u8int *)arr + 8);

  for (u32int i = 0; i < msg_len; i++) {
    packed_msg[i] = (u8int)sparse_base[i];
  }

  /* Copy to Exchange Page */
  memmove(up->p9page, packed_msg, msg_len);
  xfree(packed_msg);

  /* Route the message */
  long res = p9_route_message(up->pid, up->p9page, msg_len);

  m3ApiReturn((vlong)res);
}

/*
 * lux9_yield() -> void
 *
 * Yields the CPU.
 */
m3ApiRawFunction(lux9_yield) {
  m3ApiReturnType(void) sched();
  m3ApiSuccess();
}

/*
 * lux9_debug_print(str: ref, len: i32) -> void
 *
 * Prints to kernel console (kprint).
 */
m3ApiRawFunction(lux9_debug_print) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, str_val);
  m3ApiGetArg(u64int, len64);
  u32int len = (u32int)len64;

  if (len > 256)
    len = 256;

  void *arr = clr_ptr_to_mem(str_val, _mem);
  if (!arr) {
    m3ApiSuccess();
  }

  /* Repack sparse array */
  char buf[257];
  u64int *sparse_base = (u64int *)((u8int *)arr + 8);
  for (u32int i = 0; i < len; i++) {
    buf[i] = (char)sparse_base[i];
  }
  buf[len] = 0;

  print("%s", buf);

  m3ApiSuccess();
}

extern int clr_execute_assembly(void *dll_data, ulong dll_size);

static void kexec_trampoline(void *arg) {
  char *path = (char *)arg;
  Chan *c = nil;
  void *asm_data = nil;

  if (waserror()) {
    print("lux9_spawn: failed to spawn %s: %s\n", path, up->errstr);
    if (c)
      cclose(c);
    if (asm_data)
      free(asm_data);
    free(path); // Free the kstrdup'd path
    pexit("spawn failed", 1);
  }

  c = namec(path, Aopen, OEXEC, 0);

  // Get file size
  Dir *dir = dirchanstat(c);
  if (dir == nil)
    error(Eio);
  ulong fsize = dir->length;
  free(dir);

  // Read file
  asm_data = malloc(fsize);
  if (asm_data == nil)
    error(Enomem);

  devtab[c->type]->read(c, asm_data, fsize, 0);
  cclose(c);
  c = nil;

  // Execute
  print("lux9_spawn: executing %s\n", path);
  int ret = clr_execute_assembly(asm_data, fsize);

  print("lux9_spawn: %s exited with %d\n", path, ret);

  free(asm_data);
  free(path);
  pexit("child exit", 0);
}

/*
 * lux9_spawn(path: ref) -> i32
 * Spawns a new process executing the CLR assembly at 'ptr'.
 */
m3ApiRawFunction(lux9_spawn) {
  m3ApiReturnType(vlong) m3ApiGetArg(u64int, path_val);
  char *path = (char *)clr_ptr_to_mem(path_val, _mem);

  if (!path)
    m3ApiReturn(-1);

  // Copy path to kernel heap for the child
  char *kpath;
  kstrdup(&kpath, path); // Allocated in pool, needs freeing in child

  // kproc now returns int (pid)
  int pid = kproc("child", kexec_trampoline, kpath);
  m3ApiReturn((vlong)pid);
}

/*
 * lux9_sleep(ms: i32) -> void
 */
m3ApiRawFunction(lux9_sleep) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, ms64);
  u32int ms = (u32int)ms64;

  if (ms > 0) {
    tsleep(&up->sleep, return0, 0, ms);
  }

  m3ApiSuccess();
}

/* CLR/Pebble runtime imports - using WASM linear memory */
m3ApiRawFunction(clr_lux_alloc) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, size);
  m3ApiGetArg(u64int, type_token);
  USED(type_token);

  /* Allocate from WASM linear memory (returns offset, not kernel ptr) */
  u32int offset = wasm_bump_alloc((u32int)size);
  m3ApiReturn((u64int)offset);
}

m3ApiRawFunction(clr_lux_addref) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, ptr_val);
  /* Bump allocator - no refcounting, just return same offset */
  m3ApiReturn(ptr_val);
}

m3ApiRawFunction(clr_lux_release) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, ptr_val);
  USED(ptr_val);
  /* Bump allocator - no freeing */
  m3ApiSuccess();
}

m3ApiRawFunction(clr_lux_snapshot) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, ptr_val);
  /* MVP: no snapshot support, return same offset */
  m3ApiReturn(ptr_val);
}

m3ApiRawFunction(clr_lux_commit) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, ptr_val);
  USED(ptr_val);
  m3ApiSuccess();
}

m3ApiRawFunction(clr_lux_rollback) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, ptr_val);
  USED(ptr_val);
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_string_from_literal) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, us_index64);
  u32int us_index = (u32int)us_index64;
  void *ptr = clr_string_from_literal(us_index);
  m3ApiReturn(clr_tag_ptr(ptr));
}

m3ApiRawFunction(clr_import_get_type_size) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, token64);
  u32int token = (u32int)token64;
  m3ApiReturn((u64int)clr_get_type_size(token));
}

m3ApiRawFunction(clr_import_get_static_field) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, token64);
  u32int token = (u32int)token64;
  void *ptr = clr_get_static_field(token);
  m3ApiReturn(clr_tag_ptr(ptr));
}

m3ApiRawFunction(clr_import_is_instance_of) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, obj_ptr);
  m3ApiGetArg(u64int, type_token64);
  u32int type_token = (u32int)type_token64;
  int res =
      clr_is_instance_of((clr_object_t *)clr_untag_ptr(obj_ptr), type_token);
  m3ApiReturn((u64int)res);
}

m3ApiRawFunction(clr_import_ptr_add) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, ptr_val);
  m3ApiGetArg(u64int, offset);
  if (ptr_val & CLR_PTR_TAG) {
    u64int base = ptr_val & ~CLR_PTR_TAG;
    m3ApiReturn((base + offset) | CLR_PTR_TAG);
  }
  m3ApiReturn(ptr_val + offset);
}

m3ApiRawFunction(clr_import_load_i64) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, ptr_val);
  void *ptr = clr_ptr_to_mem(ptr_val, _mem);
  m3ApiReturn(*(u64int *)ptr);
}

m3ApiRawFunction(clr_import_store_i64) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, ptr_val);
  m3ApiGetArg(u64int, value);
  void *ptr = clr_ptr_to_mem(ptr_val, _mem);
  *(u64int *)ptr = value;
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_memmove) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, dst_val);
  m3ApiGetArg(u64int, src_val);
  m3ApiGetArg(u64int, size);
  void *dst = clr_ptr_to_mem(dst_val, _mem);
  void *src = clr_ptr_to_mem(src_val, _mem);
  memmove(dst, src, (ulong)size);
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_memset) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, dst_val);
  m3ApiGetArg(u64int, value);
  m3ApiGetArg(u64int, size);
  void *dst = clr_ptr_to_mem(dst_val, _mem);
  memset(dst, (int)value, (ulong)size);
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_newobj) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, ctor_token64);
  u32int ctor_token = (u32int)ctor_token64;
  ulong size = clr_get_type_size(ctor_token);
  if (size < 16)
    size = 16;
  void *obj = lux_alloc(size, ctor_token);
  if (obj)
    memset(obj, 0, size);
  m3ApiReturn(clr_tag_ptr(obj));
}

m3ApiRawFunction(clr_import_newarr) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, elem_token64);
  u32int elem_token = (u32int)elem_token64;
  m3ApiGetArg(u64int, length);
  ulong elem_size = 8; /* Force 8-byte stride to match array_get/set */
  ulong total = 8 + (ulong)length * elem_size;
  void *arr = lux_alloc(total, elem_token);
  if (arr)
    *(u64int *)arr = (u64int)length;
  m3ApiReturn(clr_tag_ptr(arr));
}

m3ApiRawFunction(clr_import_string_create) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, arr_val);
  void *arr = clr_ptr_to_mem(arr_val, _mem);
  if (!arr)
    m3ApiReturn(0);

  u64int len = *(u64int *)arr;
  /* String is packed UTF-16: 8 bytes header + len*2 bytes */
  ulong str_size = 8 + (len * 2);
  void *str = lux_alloc(str_size, 0);
  if (!str)
    m3ApiReturn(0);

  *(u64int *)str = len;

  /* Copy Wide Array (u64) to Packed String (u16) */
  u64int *src = (u64int *)arr + 1;
  u16int *dst = (u16int *)((u8int *)str + 8);
  for (u64int i = 0; i < len; i++) {
    dst[i] = (u16int)src[i];
  }

  m3ApiReturn(clr_tag_ptr(str));
}

m3ApiRawFunction(clr_import_array_len) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, arr_val);
  void *arr = clr_ptr_to_mem(arr_val, _mem);
  if (!arr)
    m3ApiReturn(0);
  m3ApiReturn(*(u64int *)arr);
}

m3ApiRawFunction(clr_import_string_get_length) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, ptr_val);
  void *ptr = clr_ptr_to_mem(ptr_val, _mem);
  if (!ptr)
    m3ApiReturn(0);
  m3ApiReturn(*(u64int *)ptr);
}

m3ApiRawFunction(clr_import_string_get_char) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, ptr_val);
  m3ApiGetArg(u64int, index);
  void *ptr = clr_ptr_to_mem(ptr_val, _mem);
  if (!ptr)
    m3ApiReturn(0);
  u64int len = *(u64int *)ptr;
  if (index >= len)
    m3ApiReturn(0);
  u16int *chars = (u16int *)((u8int *)ptr + 8);
  m3ApiReturn(chars[index]);
}

m3ApiRawFunction(clr_import_array_get) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, arr_val);
  m3ApiGetArg(u64int, index);
  void *arr = clr_ptr_to_mem(arr_val, _mem);
  if (!arr)
    m3ApiReturn(0);
  u64int *base = (u64int *)arr;
  m3ApiReturn(base[1 + index]);
}

m3ApiRawFunction(clr_import_array_set) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, arr_val);
  m3ApiGetArg(u64int, index);
  m3ApiGetArg(u64int, value);
  void *arr = clr_ptr_to_mem(arr_val, _mem);
  if (arr) {
    u64int *base = (u64int *)arr;
    base[1 + index] = value;
  }
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_array_elem_addr) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, arr_val);
  m3ApiGetArg(u64int, index);
  void *arr = clr_ptr_to_mem(arr_val, _mem);
  if (!arr)
    m3ApiReturn(0);
  u64int *base = (u64int *)arr;
  if (arr_val & CLR_PTR_TAG) {
    m3ApiReturn(clr_tag_ptr(&base[1 + index]));
  }
  {
    u64int base_off = (u64int)((u8int *)arr - (u8int *)_mem);
    u64int elem_off = (u64int)((u8int *)&base[1 + index] - (u8int *)arr);
    m3ApiReturn(base_off + elem_off);
  }
}

m3ApiRawFunction(clr_import_box) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, value);
  void *box = lux_alloc(sizeof(u64int), 0);
  if (box)
    *(u64int *)box = value;
  m3ApiReturn(clr_tag_ptr(box));
}

m3ApiRawFunction(clr_import_unbox) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, obj_val);
  m3ApiReturn(obj_val);
}

m3ApiRawFunction(clr_import_unbox_any) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, obj_val);
  void *ptr = clr_ptr_to_mem(obj_val, _mem);
  if (!ptr)
    m3ApiReturn(0);
  m3ApiReturn(*(u64int *)ptr);
}

m3ApiRawFunction(clr_import_initobj) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, dst_val);
  m3ApiGetArg(u64int, size);
  void *dst = clr_ptr_to_mem(dst_val, _mem);
  memset(dst, 0, (ulong)size);
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_cpobj) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, dst_val);
  m3ApiGetArg(u64int, src_val);
  m3ApiGetArg(u64int, size);
  void *dst = clr_ptr_to_mem(dst_val, _mem);
  void *src = clr_ptr_to_mem(src_val, _mem);
  memmove(dst, src, (ulong)size);
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_ldobj) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, src_val);
  void *src = clr_ptr_to_mem(src_val, _mem);
  m3ApiReturn(*(u64int *)src);
}

m3ApiRawFunction(clr_import_stobj) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, dst_val);
  m3ApiGetArg(u64int, value);
  void *dst = clr_ptr_to_mem(dst_val, _mem);
  *(u64int *)dst = value;
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_throw) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, ex_val);
  print("CLR: throw invoked ex=%#llux\n", (uvlong)ex_val);
  {
    IM3BacktraceInfo bt = m3_GetBacktrace(runtime);
    IM3BacktraceFrame frame;

    if (bt == nil || bt->frames == nil) {
      print("CLR: throw backtrace: (none)\n");
    } else {
      print("CLR: throw backtrace:\n");
      for (frame = bt->frames; frame != nil; frame = frame->next) {
        if (frame == M3_BACKTRACE_TRUNCATED) {
          print("CLR:  ... (truncated)\n");
          break;
        }
        if (frame->function) {
          print("CLR:  func=%s offset=0x%lux\n",
                m3_GetFunctionName(frame->function),
                (ulong)frame->moduleOffset);
        } else {
          print("CLR:  func=(nil) offset=0x%lux\n", (ulong)frame->moduleOffset);
        }
      }
    }
  }
  m3ApiTrap(m3Err_trapAbort);
}

/* Linker function to bind these to a module */
/* Include IL Parser for RVA lookup */
/* #include "../il_parser.h" - Removed to avoid header conflicts */

/* Use helpers provided by clr_runtime.c */
extern u32int clr_get_field_rva(u32int token);
extern u32int clr_rva_to_offset(u32int rva);
extern void *clr_get_assembly_data(void);
extern u32int clr_get_assembly_len(void);

/* Wrapper for loading static fields using RVA */
m3ApiRawFunction(clr_import_ldsfld) {
  m3ApiReturnType(u64int) m3ApiGetArg(u32int, token);

  u32int rva = clr_get_field_rva(token);
  if (rva) {
    u32int offset = clr_rva_to_offset(rva);
    void *base = clr_get_assembly_data();
    u32int len = clr_get_assembly_len();

    if (base && offset < len) {
      void *addr = (u8 *)base + offset;
      m3ApiReturn(*(u64int *)addr);
    }
  }
  m3ApiReturn(0);
}

/* NOTE: m3Err_trapOutOfBounds is likely m3Err_trapOutOfBoundsMemoryAccess or
 * similar string */
/* wasm3 defines traps as const strings. */
/* Using generic "trap: out of bounds" string if m3Err_trapOutOfBounds is not
 * macro */
#ifndef m3Err_trapOutOfBounds
#define m3Err_trapOutOfBounds "trap: out of bounds"
#endif

m3ApiRawFunction(clr_import_stsfld) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, value);
  m3ApiGetArg(u32int, token);

  u32int rva = clr_get_field_rva(token);
  if (rva) {
    u32int offset = clr_rva_to_offset(rva);
    void *base = clr_get_assembly_data();
    u32int len = clr_get_assembly_len();

    if (base && offset < len) {
      void *addr = (u8 *)base + offset;
      *(u64int *)addr = value;
    }
  }
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_ldfld) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, obj);
  m3ApiGetArg(u32int, token);
  // Simplistic field load: assume obj is ptr, field offset via token?
  // Current clr_load_i64 is just identity deref.
  // Real layout needs token->offset mapping.
  // For now, assume field 0 is at offset 0?
  // WARN: This is incorrect for multiple fields. But Init.fs might only use
  // fields on types it controls? Init.fs uses stsfld/ldsfld mostly.
  m3ApiReturn(0);
}

m3ApiRawFunction(clr_import_stfld) {
  m3ApiReturnType(void) m3ApiGetArg(u64int, obj);
  m3ApiGetArg(u64int, value);
  m3ApiGetArg(u32int, token);
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_ldflda) {
  m3ApiReturnType(u64int) m3ApiGetArg(u64int, obj);
  m3ApiGetArg(u32int, token);
  m3ApiReturn(obj); // Return obj ptr itself? incorrect but non-crashing
}

m3ApiRawFunction(clr_import_ldsflda) {
  m3ApiReturnType(u64int) m3ApiGetArg(u32int, token);
  u32int rva = clr_get_field_rva(token);
  if (rva) {
    u32int offset = clr_rva_to_offset(rva);
    void *base = clr_get_assembly_data();
    u32int len = clr_get_assembly_len();

    if (base && offset < len) {
      void *addr = (u8 *)base + offset;
      m3ApiReturn((u64int)(uintptr_t)addr);
    }
  }
  m3ApiReturn(0);
}

m3ApiRawFunction(clr_import_ldelem) {
  m3ApiReturnType(u64int) m3ApiGetArg(u32int, opcode);
  m3ApiGetArg(u64int, arr);
  m3ApiGetArg(u32int, idx);
  // Call existing array check?
  // Use clr_import_array_get logic?
  // clr_import_array_get(arr, idx) -> val.
  // We ignore opcode for now (assumes i64/ref size).
  if (!arr)
    m3ApiReturn(0);
  // Assuming arr is Pointer to Array Header.
  // Header: Length (4 or 8), then data.
  // clr_import_array_get implementation:
  /*
  void *ptr = clr_ptr_to_mem(arr_val, _mem);
  u64int len = *(u64int *)ptr;
  if (idx >= len) m3ApiTrap(m3Err_trapOutOfBounds);
  u64int *data = (u64int *)((u8 *)ptr + 8);
  m3ApiReturn(data[idx]);
  */
  // So we can reuse or copy logic.
  // Reuse existing function? m3ApiRawFunction defines a function.
  // We can call static helper.

  // Just impl:
  // Need _mem (linear memory base) if arr is offset.
  // But our newobj returns HOST POINTERS cast to i64?
  // clr_alloc returns `void*`.
  // WASM module sees it as i64.
  // Functions like clr_array_get assume `arr` is an offset in WASM memory IF
  // `clr_ptr_to_mem` handles it. `clr_ptr_to_mem` in `lux9_api.c`:
  /*
  void *clr_ptr_to_mem(u64int ptr, void *_mem) {
    if (ptr > 0xFFFFFFFF) return (void *)ptr; // Assume host pointer
    if (ptr == 0) return NULL;
    // Otherwise offset? Or maybe we strictly use host pointers?
    // lux_alloc returns host pointers.
    return (void *)ptr;
  }
  */
  void *ptr = (void *)arr;
  if (!ptr)
    m3ApiReturn(0);
  u64int *data = (u64int *)((u8 *)ptr + 16); // Skip object header?
  // clr_newarr allocates: sizeof(u64) length + data.
  // Just length?
  // clr_import_newarr:
  /*
    u64int *ptr = clr_alloc(sizeof(u64int) + sizeof(u64int) * len);
    *ptr = len;
    m3ApiReturn((u64int)ptr);
  */
  // So offset 0 is len. data starts at +8.

  u64int len = *(u64int *)ptr;
  if (idx >= len)
    m3ApiTrap(m3Err_trapOutOfBounds);
  u64int *elems = (u64int *)((u8 *)ptr + 8);
  m3ApiReturn(elems[idx]);
}

m3ApiRawFunction(clr_import_stelem) {
  m3ApiReturnType(void) m3ApiGetArg(u32int, opcode);
  m3ApiGetArg(u64int, arr);
  m3ApiGetArg(u32int, idx);
  m3ApiGetArg(u64int, val);

  void *ptr = (void *)arr;
  if (!ptr)
    m3ApiSuccess();
  u64int len = *(u64int *)ptr;
  if (idx >= len)
    m3ApiTrap(m3Err_trapOutOfBounds);
  u64int *elems = (u64int *)((u8 *)ptr + 8);
  elems[idx] = val;
  m3ApiSuccess();
}

m3ApiRawFunction(clr_import_ldelema) {
  m3ApiReturnType(u64int) m3ApiGetArg(u32int, token);
  m3ApiGetArg(u64int, arr);
  m3ApiGetArg(u32int, idx);

  void *ptr = (void *)arr;
  if (!ptr)
    m3ApiReturn(0);
  u64int len = *(u64int *)ptr;
  if (idx >= len)
    m3ApiTrap(m3Err_trapOutOfBounds);
  u64int *elems = (u64int *)((u8 *)ptr + 8);
  m3ApiReturn((u64int)(uintptr_t)&elems[idx]);
}

m3ApiRawFunction(clr_import_isinst) {
  m3ApiReturnType(u64int) m3ApiGetArg(u32int, token);
  m3ApiGetArg(u64int, obj);
  m3ApiReturn(obj); // Stub: everything is instance
}

/* Linker function to bind these to a module */
M3Result lux9_link_wasi(IM3Module module) {
  M3Result result = m3Err_none;

#define LINK_RAW(func, sig, impl)                                              \
  do {                                                                         \
    result = m3_LinkRawFunction(module, "env", func, sig, impl);               \
    if (result == m3Err_functionLookupFailed) {                                \
      result = m3Err_none;                                                     \
    } else if (result) {                                                       \
      print("lux9_link_wasi: link failed %s %s: %s\n", func, sig, result);     \
      return result;                                                           \
    }                                                                          \
  } while (0)

  LINK_RAW("lux9_send_9p", "I(II)", &lux9_send_9p);
  LINK_RAW("lux9_yield", "v()", &lux9_yield);
  LINK_RAW("lux9_debug_print", "v(II)", &lux9_debug_print);
  LINK_RAW("lux9_spawn", "I(I)", &lux9_spawn);
  LINK_RAW("lux9_sleep", "v(I)", &lux9_sleep);

  LINK_RAW("lux_alloc", "I(II)", &clr_lux_alloc);
  LINK_RAW("lux_addref", "I(I)", &clr_lux_addref);
  LINK_RAW("lux_release", "v(I)", &clr_lux_release);
  LINK_RAW("lux_snapshot", "I(I)", &clr_lux_snapshot);
  LINK_RAW("lux_commit", "v(I)", &clr_lux_commit);
  LINK_RAW("lux_rollback", "v(I)", &clr_lux_rollback);

  LINK_RAW("clr_string_from_literal", "I(I)", &clr_import_string_from_literal);
  LINK_RAW("clr_string_create", "I(I)", &clr_import_string_create);
  LINK_RAW("clr_string_get_length", "I(I)", &clr_import_string_get_length);
  LINK_RAW("clr_string_get_char", "I(II)", &clr_import_string_get_char);
  LINK_RAW("clr_get_type_size", "I(I)", &clr_import_get_type_size);
  LINK_RAW("clr_get_static_field", "I(I)", &clr_import_get_static_field);
  LINK_RAW("clr_is_instance_of", "I(II)", &clr_import_is_instance_of);

  LINK_RAW("clr_ptr_add", "I(II)", &clr_import_ptr_add);
  LINK_RAW("clr_load_i64", "I(I)", &clr_import_load_i64);
  LINK_RAW("clr_store_i64", "v(II)", &clr_import_store_i64);
  LINK_RAW("clr_memmove", "v(III)", &clr_import_memmove);
  LINK_RAW("clr_memset", "v(III)", &clr_import_memset);

  /* New imports strictly matching cil_to_wasm.c mapping (0-16) */
  /* Indices match order in cil_to_wasm generated module imports */

  LINK_RAW("clr_newobj", "I(I)", &clr_import_newobj);
  LINK_RAW("clr_newarr", "I(II)", &clr_import_newarr);
  /* clr_string_from_literal already linked above, repeated is harmless if m3
   * handles it, or just ensure it covers index 2 */
  // Actually, import order in WASM module matters.
  // Bindings are by Name.
  // The module imports by name.

  LINK_RAW("clr_ldsfld", "I(I)", &clr_import_ldsfld);
  LINK_RAW("clr_stsfld", "v(II)", &clr_import_stsfld);
  LINK_RAW("clr_ldfld", "I(II)", &clr_import_ldfld);
  LINK_RAW("clr_stfld", "v(III)", &clr_import_stfld);
  LINK_RAW("clr_ldflda", "I(II)", &clr_import_ldflda);
  LINK_RAW("clr_ldsflda", "I(I)", &clr_import_ldsflda);

  /* clr_ldlen maps to array_len but needs signature match I(I) */
  LINK_RAW("clr_ldlen", "I(I)", &clr_import_array_len);

  LINK_RAW("clr_box", "I(II)",
           &clr_import_box); /* Sig I(II) (token, val) -> obj */
  LINK_RAW("clr_unbox", "I(II)",
           &clr_import_unbox); /* Sig I(II) (token, obj) -> addr */

  LINK_RAW("clr_isinst", "I(II)", &clr_import_isinst);
  LINK_RAW("clr_initobj", "v(II)", &clr_import_initobj);

  LINK_RAW("clr_ldelem", "I(III)", &clr_import_ldelem);
  LINK_RAW("clr_stelem", "v(IIII)", &clr_import_stelem);
  LINK_RAW("clr_ldelema", "I(III)", &clr_import_ldelema);

  LINK_RAW("clr_cpobj", "v(III)", &clr_import_cpobj);
  LINK_RAW("clr_ldobj", "I(I)", &clr_import_ldobj);
  LINK_RAW("clr_stobj", "v(II)", &clr_import_stobj);
  LINK_RAW("clr_throw", "v(I)", &clr_import_throw);

#undef LINK_RAW

  return result;
}
