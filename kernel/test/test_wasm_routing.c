/*
 * test_wasm_routing.c - Unit Tests for WASM 9P Routing Logic
 *
 * Mocks kernel dependencies (msgord, ledger) to test the routing policy layer.
 */

#include "../include/blind_ledger.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/lib.h"
#include "../include/u.h"
#include "../wasm/wasm_9p_integration.h"

/* --- Test Framework & Mocks --- */

#define TEST_ASSERT(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      print("FAIL: %s\n", msg);                                                \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#define TEST_PASS(name) print("PASS: %s\n", name)

/* Mock State */
static uuid_t last_submitted_uuid;
static int mock_fs_submit_called = 0;

/* Mock: wasm_fs_submit */
/* We define this weak or just include this file carefully to avoid collision.
 * Since this is a standalone test file likely not linked with wasm_fileserver.c
 * in a full kernel build without guards, we might need a separate build target
 * or assume partial linkage. For now, we assume this file is built INSTEAD of
 * wasm_fileserver.c for a test binary, OR we rely on linker preemption if
 * supported.
 *
 * Better approach: Function pointers or hooks.
 * But given the environment, let's redefine with a mock prefix and manually
 * include the source under test if we were building a separate binary.
 *
 * Since I'm adding this to `kernel/test/`, and `kernel/Makefile` builds
 * *everything* into `lux9.elf`, defining `wasm_fs_submit` here will conflict
 * with `wasm/wasm_fileserver.c`.
 *
 * Strategy: I will rely on the real `wasm_9p_integration.c` which calls
 * `wasm_fs_submit`. `wasm_fs_submit` is in `wasm_fileserver.c`. I can't easily
 * mock it without affecting the global build.
 *
 * ALTERNATIVE: Test `wasm_9p_required_perms` and `wasm_9p_extract_cap_uuid`
 * purely. Testing `wasm_9p_route_to_wasm` requires integration.
 *
 * Let's implement full integration with a valid dummy session.
 *
 * If I populate a session with a `wasm_server` pointer that points to a dummy
 * structure, `wasm_fs_submit` will try to use it. `wasm_fs_submit` does
 * validation: `if (!server || !caller ...)` It then calls `pool_prepare_hybrid`
 * and map/unmap. This is hard to fake in a running kernel without crashing.
 *
 * DECISION: Limit scope to Testing `wasm_9p_integration.c` helper functions
 * and Session Registry logic (RB-Tree), skipping the final `wasm_fs_submit`
 * call.
 */

/* --- Tests --- */

static int test_perm_mapping(void) {
  TEST_ASSERT(wasm_9p_required_perms(WASM_9P_OP_READ, 0) == CAP_PERM_READ,
              "READ op needs READ perm");
  TEST_ASSERT(wasm_9p_required_perms(WASM_9P_OP_WRITE, 0) == CAP_PERM_WRITE,
              "WRITE op needs WRITE perm");
  TEST_ASSERT(wasm_9p_required_perms(WASM_9P_OP_OPEN, OREAD) == CAP_PERM_READ,
              "OPEN(OREAD) needs READ perm");
  TEST_ASSERT(wasm_9p_required_perms(WASM_9P_OP_OPEN, ORDWR) ==
                  (CAP_PERM_READ | CAP_PERM_WRITE),
              "OPEN(ORDWR) needs R|W perm");

  TEST_PASS("test_perm_mapping");
  return 0;
}

static int test_uuid_parsing(void) {
  uuid_t u;
  const char *valid = "capid=00112233445566778899aabbccddeeff";
  TEST_ASSERT(wasm_9p_extract_cap_uuid(valid, &u) == 0, "Valid UUID parse");
  TEST_ASSERT(u.data[0] == 0x00, "UUID byte 0");
  TEST_ASSERT(u.data[15] == 0xff, "UUID byte 15");

  TEST_ASSERT(wasm_9p_extract_cap_uuid("capid=invalid", &u) != 0,
              "Invalid length rejected");
  TEST_ASSERT(wasm_9p_extract_cap_uuid("nocap_prefix", &u) != 0,
              "Missing prefix rejected");

  TEST_PASS("test_uuid_parsing");
  return 0;
}

static int test_session_registry(void) {
  uuid_t dummy_uuid = {{0}};
  void *dummy_server = (void *)0xDEADBEEF;

  /* Create */
  wasm_9p_session_t *s1 = wasm_9p_create_session(&dummy_uuid, dummy_server);
  TEST_ASSERT(s1 != nil, "Session creation");
  TEST_ASSERT(s1->session_id > 0, "Session ID assigned");

  u32int sid = s1->session_id;

  /* Lookup */
  wasm_9p_session_t *s2 = wasm_9p_get_session(sid); // refs -> 2
  TEST_ASSERT(s2 == s1, "Session lookup returns same obj");
  TEST_ASSERT(s2->ref == 2, "Refcount incremented");

  /* Register Alias */
  TEST_ASSERT(wasm_router_register(sid, "test_srv") == 0, "Register alias");
  TEST_ASSERT(strcmp(s1->name, "test_srv") == 0, "Name stored");

  /* Cleanup */
  wasm_9p_session_unref(sid);  // refs -> 1
  wasm_9p_destroy_session(s1); // refs -> 0, free

  /* Verify Gone */
  TEST_ASSERT(wasm_9p_get_session(sid) == nil, "Session gone after destroy");

  TEST_PASS("test_session_registry");
  return 0;
}

int test_wasm_routing_main(void) {
  int fail = 0;
  print("\n=== WASM 9P Routing Tests ===\n\n");

  if (test_perm_mapping() != 0)
    fail++;
  if (test_uuid_parsing() != 0)
    fail++;
  if (test_session_registry() != 0)
    fail++;

  if (!fail)
    print("=== All WASM tests passed ===\n");
  else
    print("=== FAILURES in WASM tests ===\n");

  return fail;
}
