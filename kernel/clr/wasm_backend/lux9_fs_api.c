/*
 * WASM Filesystem API Bridge
 * Provides kernel-to-WASM invocation for userspace RamFS
 */

#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../include/portlib.h"
#include "../../include/u.h"
#include "m3_env.h"
#include "wasm3.h"

/* Global RamFS WASM runtime state */
static IM3Runtime ramfs_runtime = NULL;
static IM3Module ramfs_module = NULL;
static IM3Function ramfs_create_fn = NULL;
static IM3Function ramfs_read_fn = NULL;
static IM3Function ramfs_write_fn = NULL;
static IM3Function ramfs_remove_fn = NULL;
static IM3Function ramfs_stat_fn = NULL;

/*
 * Initialize RamFS WASM instance
 * Called when RamFS.dll is loaded
 */
int ramfs_init_wasm(IM3Runtime runtime, IM3Module module) {
  ramfs_runtime = runtime;
  ramfs_module = module;

  /* Lookup RamFS function exports */
  M3Result result;

  /* Note: F# exports will be mangled, need to find actual names */
  /* For now, we'll use direct WASM memory for simple IPC */

  print("ramfs_wasm: Initialized\n");
  return 0;
}

/*
 * Simple shared memory IPC mechanism
 * RamFS operations use a command buffer for requests/responses
 */
#define RAMFS_CMD_CREATE 1
#define RAMFS_CMD_READ 2
#define RAMFS_CMD_WRITE 3
#define RAMFS_CMD_REMOVE 4
#define RAMFS_CMD_STAT 5
#define RAMFS_CMD_LIST 6

typedef struct {
  int cmd;
  char path[256];
  union {
    struct {
      int is_dir;
      int perm;
    } create;
    struct {
      uint64_t offset;
      uint32_t count;
      uint32_t result_len;
    } read;
    struct {
      uint64_t offset;
      uint32_t count;
    } write;
    struct {
      int is_dir;
      int64_t size;
      int perm;
    } stat;
  } args;
  int result;
  char data[8192]; /* For read/write data */
} RamfsCommand;

static RamfsCommand ramfs_cmd_buf;

/*
 * Simplified approach: Use direct function calls to F# RamFS instance
 * The RamFS.dll will stay resident and we access it via global instance
 */

/* External reference to loaded RamFS instance */
extern void *get_ramfs_instance(void); /* Will be implemented in CLR runtime */

/*
 * Invoke RamFS Create operation
 */
int ramfs_invoke_create(const char *path, int is_dir, int perm) {
  if (ramfs_runtime == NULL) {
    print("ramfs_wasm: Runtime not initialized\n");
    return -1;
  }

  /* Prepare command */
  ramfs_cmd_buf.cmd = RAMFS_CMD_CREATE;
  strncpy(ramfs_cmd_buf.path, path, sizeof(ramfs_cmd_buf.path) - 1);
  ramfs_cmd_buf.args.create.is_dir = is_dir;
  ramfs_cmd_buf.args.create.perm = perm;

  /* For now, just log - actual WASM invocation will be added */
  print("ramfs_wasm: Create %s %s (perm %o)\n", path, is_dir ? "dir" : "file",
        perm);

  return 0;
}

/*
 * Invoke RamFS Read operation
 */
int ramfs_invoke_read(const char *path, uint64_t offset, void *buf,
                      uint32_t count) {
  if (ramfs_runtime == NULL)
    return -1;

  ramfs_cmd_buf.cmd = RAMFS_CMD_READ;
  strncpy(ramfs_cmd_buf.path, path, sizeof(ramfs_cmd_buf.path) - 1);
  ramfs_cmd_buf.args.read.offset = offset;
  ramfs_cmd_buf.args.read.count = count;

  print("ramfs_wasm: Read %s at offset %lld, count %u\n", path, offset, count);

  /* TODO: Actual WASM invocation */
  /* For now return empty */
  return 0;
}

/*
 * Invoke RamFS Write operation
 */
int ramfs_invoke_write(const char *path, uint64_t offset, const void *data,
                       uint32_t count) {
  if (ramfs_runtime == NULL)
    return -1;

  ramfs_cmd_buf.cmd = RAMFS_CMD_WRITE;
  strncpy(ramfs_cmd_buf.path, path, sizeof(ramfs_cmd_buf.path) - 1);
  ramfs_cmd_buf.args.write.offset = offset;
  ramfs_cmd_buf.args.write.count = count;

  if (count > sizeof(ramfs_cmd_buf.data))
    count = sizeof(ramfs_cmd_buf.data);

  memmove(ramfs_cmd_buf.data, data, count);

  print("ramfs_wasm: Write %s at offset %lld, count %u\n", path, offset, count);

  /* TODO: Actual WASM invocation */
  return count;
}

/*
 * Invoke RamFS Remove operation
 */
int ramfs_invoke_remove(const char *path) {
  if (ramfs_runtime == NULL)
    return -1;

  ramfs_cmd_buf.cmd = RAMFS_CMD_REMOVE;
  strncpy(ramfs_cmd_buf.path, path, sizeof(ramfs_cmd_buf.path) - 1);

  print("ramfs_wasm: Remove %s\n", path);

  /* TODO: Actual WASM invocation */
  return 0;
}

/*
 * Invoke RamFS Stat operation
 */
int ramfs_invoke_stat(const char *path, int *is_dir, int64_t *size, int *perm) {
  if (ramfs_runtime == NULL)
    return -1;

  ramfs_cmd_buf.cmd = RAMFS_CMD_STAT;
  strncpy(ramfs_cmd_buf.path, path, sizeof(ramfs_cmd_buf.path) - 1);

  print("ramfs_wasm: Stat %s\n", path);

  /* TODO: Actual WASM invocation */
  /* For now, fake a directory */
  *is_dir = 1;
  *size = 0;
  *perm = 0755;

  return 0;
}
