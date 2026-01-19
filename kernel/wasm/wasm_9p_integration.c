/* wasm_9p_integration.c - 9P + Capability Integration Implementation
 *
 * Implements validation and routing of 9P messages with capability checks.
 * Integrates Pebble capabilities (UUID-encoded) with 9P protocol.
 */

#include "wasm_9p_integration.h"
#include "../capability/lux_capability.h"
#include "../include/dat.h"
#include "../include/fcall.h"
#include "../include/fns.h"
#include "../include/mem.h"
#include "../include/portlib.h"
#include "../include/u.h"
#include "wasm_fileserver.h"

/* Global capability manager (defined in kernel init) */
extern lux_capability_manager_t *global_cap_manager;

#ifndef nil
#define nil ((void *)0)
#endif

/* Helper: Convert hex char to nibble */
/*@
  @ assigns \nothing;
  @*/
static int hex_to_nibble(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

/* Helper: Parse hex UUID string to binary */
/*@
  @ requires hex == \null || \valid(hex);
  @ requires uuid == \null || \valid(uuid);
  @ assigns \nothing;
  @*/
static int parse_uuid_hex(const char *hex, uuid_t *uuid) {
  if (!hex || !uuid)
    return -1;

  /* UUID hex string: 32 characters (16 bytes * 2 hex/byte) */
  /* Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (with optional dashes) */

  int byte_idx = 0;
    /*@ loop invariant 0 <= i <= 36 && hex[i] && byte_idx;
    @ loop assigns i;
    @ loop variant 36 && hex[i] && byte_idx - i;
    @*/
  for (int i = 0; i < 36 && hex[i] && byte_idx < 16; i++) {
    if (hex[i] == '-')
      continue; /* Skip dashes */

    int hi = hex_to_nibble(hex[i]);
    if (hi < 0)
      return -1;

    i++;
    if (!hex[i])
      return -1;

    int lo = hex_to_nibble(hex[i]);
    if (lo < 0)
      return -1;

    uuid->data[byte_idx++] = (hi << 4) | lo;
  }

  return (byte_idx == 16) ? 0 : -1;
}

/* ========== 9P Capability Extraction ========== */

/*@
  @ requires aname == \null || \valid(aname);
  @ requires uuid_out == \null || \valid(uuid_out);
  @ assigns \nothing;
  @*/
int wasm_9p_extract_cap_uuid(const char *aname, uuid_t *uuid_out) {
  if (!aname || !uuid_out)
    return -1;

  /* Check for "capid=" prefix */
  if (strncmp((char *)aname, WASM_9P_CAP_PREFIX, WASM_9P_CAP_PREFIX_LEN) == 0) {
    /* Parse hex UUID after prefix */
    return parse_uuid_hex(aname + WASM_9P_CAP_PREFIX_LEN, uuid_out);
  }

  /* Check if aname is exactly 16 bytes (binary UUID) */
  if (strlen((char *)aname) == 16) {
    memmove(uuid_out->data, aname, 16);
    return 0;
  }

  /* Check if aname is 32 hex chars (UUID without prefix) */
  if (strlen((char *)aname) == 32 || strlen((char *)aname) == 36) {
    return parse_uuid_hex(aname, uuid_out);
  }

  return -1; /* No valid UUID found */
}

/* ========== Capability Validation ========== */

int wasm_9p_validate_capability(const uuid_t *uuid, u32int required_perms,
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

  print("9p: validating cap token=%u gen=%u idx=%u perms=0x%x\n", token,
        generation, index, required_perms);

  if (!global_cap_manager) {
    print("9p: capability manager not initialized\n");
    return 0;
  }

  /* Lookup capability by UUID in capability manager */
  lux_capability_t *lang_cap = lux_cap_find_by_uuid(global_cap_manager, uuid);
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
    if (!lux_cap_validate_chain(global_cap_manager, lang_cap)) {
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

  print("9p: capability validated successfully (perms=0x%x)\n",
        lang_cap->permissions);
  return 1;
}

/* ========== Session Management ========== */

static wasm_9p_session_t *session_registry[MAX_WASM_SESSIONS];
static Lock session_lock;
static u32int next_session_id = 1;

wasm_9p_session_t *wasm_9p_get_session(u32int session_id) {
  if (session_id == 0)
    return nil;

  lock(&session_lock);
    /*@ loop invariant 0 <= i <= MAX_WASM_SESSIONS;
    @ loop assigns i;
    @ loop variant MAX_WASM_SESSIONS - i;
    @*/
  for (int i = 0; i < MAX_WASM_SESSIONS; i++) {
    if (session_registry[i] && session_registry[i]->session_id == session_id) {
      wasm_9p_session_t *s = session_registry[i];
      unlock(&session_lock);
      return s;
    }
  }
  unlock(&session_lock);
  return nil;
}

wasm_9p_session_t *wasm_9p_create_session(const uuid_t *uuid,
                                          void *wasm_server) {
  if (!uuid || !wasm_server)
    return nil;

  wasm_9p_session_t *session = malloc(sizeof(wasm_9p_session_t));
  if (!session)
    return nil;

  /* Copy UUID */
  uuid_copy(&session->cap_uuid, uuid);

  /* Validate and get Pebble capability */
  if (!wasm_9p_validate_capability(uuid, LUX_CAP_PERM_READ,
                                   &session->pebble_cap)) {
    free(session);
    return nil;
  }

  session->permissions = session->pebble_cap.perms;

  lock(&session_lock);
  session->session_id = next_session_id++;
  session->ref = 1; /* Initial reference */

  /* Register in table */
  int registered = 0;
    /*@ loop invariant 0 <= i <= MAX_WASM_SESSIONS;
    @ loop assigns i;
    @ loop variant MAX_WASM_SESSIONS - i;
    @*/
  for (int i = 0; i < MAX_WASM_SESSIONS; i++) {
    if (session_registry[i] == nil) {
      session_registry[i] = session;
      registered = 1;
      break;
    }
  }
  unlock(&session_lock);

  if (!registered) {
    print("9p: session registry full\n");
    free(session);
    return nil;
  }

  session->wasm_server = wasm_server;

  print("9p: created session %llu with perms=0x%x\n", session->session_id,
        session->permissions);

  return session;
}

/*@
  @ requires session == \null || \valid(session);
  @ assigns \nothing;
  @*/
void wasm_9p_destroy_session(wasm_9p_session_t *session) {
  if (!session)
    return;

  lock(&session_lock);
    /*@ loop invariant 0 <= i <= MAX_WASM_SESSIONS;
    @ loop assigns i;
    @ loop variant MAX_WASM_SESSIONS - i;
    @*/
  for (int i = 0; i < MAX_WASM_SESSIONS; i++) {
    if (session_registry[i] == session) {
      session_registry[i] = nil;
      break;
    }
  }
  unlock(&session_lock);

  print("9p: destroying session %llu\n", session->session_id);
  free(session);
}

/*@
  @ assigns \nothing;
  @*/
void wasm_9p_session_ref(u32int session_id) {
  if (session_id == 0)
    return;

  lock(&session_lock);
    /*@ loop invariant 0 <= i <= MAX_WASM_SESSIONS;
    @ loop assigns i;
    @ loop variant MAX_WASM_SESSIONS - i;
    @*/
  for (int i = 0; i < MAX_WASM_SESSIONS; i++) {
    if (session_registry[i] && session_registry[i]->session_id == session_id) {
      session_registry[i]->ref++;
      break;
    }
  }
  unlock(&session_lock);
}

/*@
  @ assigns \nothing;
  @*/
void wasm_9p_session_unref(u32int session_id) {
  if (session_id == 0)
    return;

  wasm_9p_session_t *to_free = nil;
  lock(&session_lock);
    /*@ loop invariant 0 <= i <= MAX_WASM_SESSIONS;
    @ loop assigns i;
    @ loop variant MAX_WASM_SESSIONS - i;
    @*/
  for (int i = 0; i < MAX_WASM_SESSIONS; i++) {
    if (session_registry[i] && session_registry[i]->session_id == session_id) {
      if (--session_registry[i]->ref <= 0) {
        to_free = session_registry[i];
        session_registry[i] = nil;
      }
      break;
    }
  }
  unlock(&session_lock);

  if (to_free) {
    print("9p: destroying session %llu (last ref)\n", to_free->session_id);
    free(to_free);
  }
}

/* ========== Permission Requirements ========== */

/*@
  @ assigns \nothing;
  @*/
u32int wasm_9p_required_perms(wasm_9p_operation_t op, u32int mode) {
  switch (op) {
  case WASM_9P_OP_ATTACH:
    return LUX_CAP_PERM_READ; /* Base access */

  case WASM_9P_OP_WALK:
  case WASM_9P_OP_STAT:
    return LUX_CAP_PERM_READ;

  case WASM_9P_OP_OPEN:
    /* Check mode flags (Plan 9 OREAD=0, OWRITE=1, ORDWR=2) */
    if (mode & 1) /* OWRITE or ORDWR */
      return LUX_CAP_PERM_READ | LUX_CAP_PERM_WRITE;
    return LUX_CAP_PERM_READ;

  case WASM_9P_OP_CREATE:
  case WASM_9P_OP_WRITE:
  case WASM_9P_OP_REMOVE:
  case WASM_9P_OP_WSTAT:
    return LUX_CAP_PERM_WRITE;

  case WASM_9P_OP_READ:
    return LUX_CAP_PERM_READ;

  case WASM_9P_OP_CLUNK:
    return 0; /* No permissions required for cleanup */

  default:
    return LUX_CAP_PERM_ALL; /* Unknown op: require all perms (safe default) */
  }
}

/* ========== 9P Routing ========== */

/*
 * Route validated 9P message to WASM server.
 * This is the main entry point: validates capability, then dispatches to WASM.
 *
 * @param t: 9P request
 * @param r: 9P reply
 * @param session: Active session with capability
 * @returns: 0 on success, error code otherwise
 */
/*@
  @ requires t == \null || \valid(t);
  @ requires r == \null || \valid(r);
  @ requires session == \null || \valid(session);
  @ assigns \nothing;
  @*/
int wasm_9p_route_to_wasm(Fcall *t, Fcall *r, wasm_9p_session_t *session) {
  if (!t || !r || !session || !session->wasm_server)
    return -1;

  /* Check permissions based on 9P operation */
  u32int required =
      wasm_9p_required_perms(t->type, (t->type == Topen) ? t->mode : 0);
  if ((session->permissions & required) != required) {
    r->type = Rerror;
    r->ename = "insufficient capability permissions";
    return -1;
  }

  /* Dispatch to WASM fileserver handler */
  return wasm_fs_handle_fcall((wasm_fileserver_t *)session->wasm_server, t, r);
}
