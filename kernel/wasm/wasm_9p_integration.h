/* wasm_9p_integration.h - 9P Protocol + Capability Integration for WASM
 *
 * Integrates Plan 9's 9P protocol with the capability system for WASM servers.
 * WASM modules implement 9P file servers, and capabilities are validated on
 * each 9P operation.
 *
 * Architecture:
 *   1. Client sends 9P message with embedded capability UUID
 *   2. Kernel validates UUID → unpacks Pebble (token, gen, index)
 *   3. Kernel checks capability permissions
 *   4. Kernel routes valid message to WASM server
 *   5. WASM server processes request, returns 9P response
 *
 * UUID Encoding in 9P:
 *   - Tattach: aname field or initial auth data contains UUID
 *   - Topen/Tcreate: Special "capid=" prefix in file name
 *   - Control: Write UUID to /mnt/ctl before operations
 */

#ifndef WASM_9P_INTEGRATION_H
#define WASM_9P_INTEGRATION_H

#include "../include/uuid.h"
#include "../include/blind_ledger.h"
#include "../capability/clr_capability.h"

/* Forward declarations for 9P types (from Plan 9 headers) */
typedef struct Fcall Fcall;
typedef struct Chan Chan;

/* ========== 9P Capability Message Format ========== */

/*
 * Capabilities are embedded in 9P messages using a standard encoding:
 *
 * Option 1: In aname field (Tattach)
 *   Format: "capid=<uuid-hex>" or just the 16-byte UUID binary
 *
 * Option 2: In file paths (Twalk, Topen)
 *   Format: "/mnt/fs#<uuid-hex>/path/to/file"
 *
 * Option 3: Control file (Write to /mnt/ctl)
 *   Format: Write "cap <uuid-hex>" to set capability for session
 *
 * We use Option 1 for simplicity: UUID in aname during Tattach
 */

#define WASM_9P_CAP_PREFIX "capid="
#define WASM_9P_CAP_PREFIX_LEN 6

/* Maximum UUID string length: "capid=" + 32 hex chars + null */
#define WASM_9P_CAP_MAXLEN (WASM_9P_CAP_PREFIX_LEN + 32 + 1)

/* ========== 9P Session with Capability ========== */

/*
 * Each 9P session (attach) is associated with a validated capability.
 * The session carries the Pebble capability that grants access to resources.
 */
typedef struct wasm_9p_session {
  uuid_t cap_uuid;              /* Capability UUID from attach */
  UserCapability pebble_cap;    /* Validated Pebble capability */
  u32int permissions;           /* Effective permissions */
  u64int session_id;            /* Unique session identifier */
  void *wasm_server;            /* WASM server handling this session */
} wasm_9p_session_t;

/* ========== 9P Message Validation ========== */

/*
 * Extract capability UUID from 9P Tattach message.
 * Looks for "capid=<uuid>" in aname field or binary UUID.
 *
 * @param aname: The aname field from Tattach
 * @param uuid_out: Output UUID
 * @returns: 0 on success, -1 on error
 */
int wasm_9p_extract_cap_uuid(const char *aname, uuid_t *uuid_out);

/*
 * Validate capability for 9P operation.
 * Unpacks UUID to Pebble (token, gen, index), validates against ledger.
 *
 * @param uuid: Capability UUID from message
 * @param required_perms: Required permissions for operation (CAP_PERM_READ, etc.)
 * @param pebble_cap_out: Output validated Pebble capability
 * @returns: 1 if valid, 0 if invalid/insufficient permissions
 */
int wasm_9p_validate_capability(const uuid_t *uuid,
                                u32int required_perms,
                                UserCapability *pebble_cap_out);

/*
 * Create 9P session with validated capability.
 *
 * @param uuid: Capability UUID
 * @param wasm_server: WASM server instance
 * @returns: Session pointer, or NULL on error
 */
wasm_9p_session_t *wasm_9p_create_session(const uuid_t *uuid, void *wasm_server);

/*
 * Destroy 9P session and clean up resources.
 *
 * @param session: Session to destroy
 */
void wasm_9p_destroy_session(wasm_9p_session_t *session);

/* ========== 9P Operation Permission Requirements ========== */

/*
 * Map 9P operations to required capability permissions.
 * These are checked on every operation before routing to WASM.
 */
typedef enum {
  WASM_9P_OP_ATTACH,   /* Requires: CAP_PERM_READ (base access) */
  WASM_9P_OP_WALK,     /* Requires: CAP_PERM_READ */
  WASM_9P_OP_OPEN,     /* Requires: CAP_PERM_READ or WRITE depending on mode */
  WASM_9P_OP_CREATE,   /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_READ,     /* Requires: CAP_PERM_READ */
  WASM_9P_OP_WRITE,    /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_CLUNK,    /* Requires: (none, cleanup) */
  WASM_9P_OP_REMOVE,   /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_STAT,     /* Requires: CAP_PERM_READ */
  WASM_9P_OP_WSTAT,    /* Requires: CAP_PERM_WRITE */
} wasm_9p_operation_t;

/*
 * Get required permissions for a 9P operation.
 *
 * @param op: 9P operation type
 * @param mode: Open mode (for OPEN operation, e.g., OREAD, OWRITE)
 * @returns: Required permission bitmask
 */
u32int wasm_9p_required_perms(wasm_9p_operation_t op, u32int mode);

/* ========== 9P → WASM Routing ========== */

/*
 * Route validated 9P message to WASM server.
 * This is the main entry point: validates capability, then dispatches to WASM.
 *
 * @param fcall: 9P message (Fcall structure)
 * @param session: Active session with capability
 * @returns: 0 on success, error code otherwise
 */
int wasm_9p_route_to_wasm(Fcall *fcall, wasm_9p_session_t *session);

#endif /* WASM_9P_INTEGRATION_H */
