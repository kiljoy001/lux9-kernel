/* wasm_9p_integration.c - 9P + Capability Integration Implementation
 *
 * Implements validation and routing of 9P messages with capability checks.
 * Integrates Pebble capabilities (UUID-encoded) with 9P protocol.
 */

#include "wasm_9p_integration.h"
#include "../capability/clr_capability.h"
#include "../include/u.h"
#include <string.h>

/* Global capability manager (defined in kernel init) */
extern capability_manager_t *global_cap_manager;

#ifndef nil
#define nil ((void*)0)
#endif

/* Helper: Convert hex char to nibble */
static int hex_to_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

/* Helper: Parse hex UUID string to binary */
static int parse_uuid_hex(const char *hex, uuid_t *uuid) {
  if (!hex || !uuid)
    return -1;

  /* UUID hex string: 32 characters (16 bytes * 2 hex/byte) */
  /* Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (with optional dashes) */

  int byte_idx = 0;
  for (int i = 0; i < 36 && hex[i] && byte_idx < 16; i++) {
    if (hex[i] == '-')
      continue; /* Skip dashes */

    int hi = hex_to_nibble(hex[i]);
    if (hi < 0) return -1;

    i++;
    if (!hex[i]) return -1;

    int lo = hex_to_nibble(hex[i]);
    if (lo < 0) return -1;

    uuid->data[byte_idx++] = (hi << 4) | lo;
  }

  return (byte_idx == 16) ? 0 : -1;
}

/* ========== 9P Capability Extraction ========== */

int wasm_9p_extract_cap_uuid(const char *aname, uuid_t *uuid_out) {
  if (!aname || !uuid_out)
    return -1;

  /* Check for "capid=" prefix */
  if (strncmp(aname, WASM_9P_CAP_PREFIX, WASM_9P_CAP_PREFIX_LEN) == 0) {
    /* Parse hex UUID after prefix */
    return parse_uuid_hex(aname + WASM_9P_CAP_PREFIX_LEN, uuid_out);
  }

  /* Check if aname is exactly 16 bytes (binary UUID) */
  if (strlen(aname) == 16) {
    memmove(uuid_out->data, aname, 16);
    return 0;
  }

  /* Check if aname is 32 hex chars (UUID without prefix) */
  if (strlen(aname) == 32 || strlen(aname) == 36) {
    return parse_uuid_hex(aname, uuid_out);
  }

  return -1; /* No valid UUID found */
}

/* ========== Capability Validation ========== */

int wasm_9p_validate_capability(const uuid_t *uuid,
                                u32int required_perms,
                                UserCapability *pebble_cap_out) {
  if (!uuid || !pebble_cap_out)
    return 0;

  /* Unpack Pebble metadata from UUID */
  u32int token, generation;
  u16int index;

  if (uuid_unpack_pebble(uuid, &token, &generation, &index) != 0) {
    print("9p: invalid capability UUID (not Pebble format)\n");
    return 0;
  }

  print("9p: validating cap token=%u gen=%u idx=%u perms=0x%x\n",
        token, generation, index, required_perms);

  /* Reconstruct UserCapability from UUID to verify against ledger
   * The capability hash is derived from the Pebble token.
   * We need to reconstruct it or look it up by (token, gen, index).
   *
   * Problem: UUID → (token, gen, idx) but we need the full hash to verify.
   * The hash = SHA256(process_hash || leaf_hash) which we don't have from UUID alone.
   *
   * Solution: The BlindLedger should support lookup by (token, gen, idx) OR
   * we need to store UUID → hash mapping in the capability table.
   *
   * For now, we'll use the language capability manager to lookup by UUID.
   */

  if (!global_cap_manager) {
    print("9p: capability manager not initialized\n");
    return 0;
  }

  /* Lookup capability by UUID in capability manager */
  clr_monotonic_capability_t *lang_cap = cap_find_by_uuid(global_cap_manager, uuid);
  if (!lang_cap) {
    print("9p: capability UUID not found in manager\n");
    return 0;
  }

  /* Check if capability is revoked */
  if (lang_cap->is_revoked) {
    print("9p: capability has been revoked\n");
    return 0;
  }

  /* Check permissions */
  if ((lang_cap->permissions & required_perms) != required_perms) {
    print("9p: insufficient permissions (have=0x%x, need=0x%x)\n",
          lang_cap->permissions, required_perms);
    return 0;
  }

  /* Validate derivation chain */
  if (!lang_cap->is_validated) {
    if (!cap_validate_chain(global_cap_manager, lang_cap)) {
      print("9p: capability chain validation failed\n");
      return 0;
    }
  }

  /* Build UserCapability output (map language cap to Pebble cap) */
  memset(pebble_cap_out, 0, sizeof(UserCapability));

  /* Copy UUID as first 16 bytes of hash */
  memmove(pebble_cap_out->hash, uuid->data, 16);

  pebble_cap_out->type = CAP_TYPE_IPC;
  pebble_cap_out->perms = lang_cap->permissions;
  pebble_cap_out->size = 0; /* IPC capabilities have no size */

  print("9p: capability validated successfully (perms=0x%x)\n", lang_cap->permissions);
  return 1;
}

/* ========== Session Management ========== */

static u64int next_session_id = 1;

wasm_9p_session_t *wasm_9p_create_session(const uuid_t *uuid, void *wasm_server) {
  if (!uuid || !wasm_server)
    return nil;

  wasm_9p_session_t *session = malloc(sizeof(wasm_9p_session_t));
  if (!session)
    return nil;

  /* Copy UUID */
  uuid_copy(&session->cap_uuid, uuid);

  /* Validate and get Pebble capability */
  if (!wasm_9p_validate_capability(uuid, CAP_PERM_READ, &session->pebble_cap)) {
    free(session);
    return nil;
  }

  session->permissions = session->pebble_cap.perms;
  session->session_id = next_session_id++;
  session->wasm_server = wasm_server;

  print("9p: created session %llu with perms=0x%x\n",
        session->session_id, session->permissions);

  return session;
}

void wasm_9p_destroy_session(wasm_9p_session_t *session) {
  if (!session)
    return;

  print("9p: destroying session %llu\n", session->session_id);
  free(session);
}

/* ========== Permission Requirements ========== */

u32int wasm_9p_required_perms(wasm_9p_operation_t op, u32int mode) {
  switch (op) {
  case WASM_9P_OP_ATTACH:
    return CAP_PERM_READ; /* Base access */

  case WASM_9P_OP_WALK:
  case WASM_9P_OP_STAT:
    return CAP_PERM_READ;

  case WASM_9P_OP_OPEN:
    /* Check mode flags (Plan 9 OREAD=0, OWRITE=1, ORDWR=2) */
    if (mode & 1) /* OWRITE or ORDWR */
      return CAP_PERM_READ | CAP_PERM_WRITE;
    return CAP_PERM_READ;

  case WASM_9P_OP_CREATE:
  case WASM_9P_OP_WRITE:
  case WASM_9P_OP_REMOVE:
  case WASM_9P_OP_WSTAT:
    return CAP_PERM_WRITE;

  case WASM_9P_OP_READ:
    return CAP_PERM_READ;

  case WASM_9P_OP_CLUNK:
    return 0; /* No permissions required for cleanup */

  default:
    return CAP_PERM_ALL; /* Unknown op: require all perms (safe default) */
  }
}

/* ========== 9P Routing ========== */

int wasm_9p_route_to_wasm(Fcall *fcall, wasm_9p_session_t *session) {
  if (!fcall || !session)
    return -1;

  /* TODO: Implement actual routing to WASM server
   * This will:
   *   1. Marshal Fcall to WASM-readable format
   *   2. Call WASM server's 9P handler function
   *   3. Unmarshal response back to Fcall
   *
   * For now, stub with logging
   */

  print("9p: routing message type=%d to WASM server (session %llu)\n",
        fcall->type, session->session_id);

  /* Stub: Return success */
  return 0;
}
