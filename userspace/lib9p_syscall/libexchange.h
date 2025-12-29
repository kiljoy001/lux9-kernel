#ifndef _LIBEXCHANGE_H_
#define _LIBEXCHANGE_H_

#include <u.h>
#include <blind_ledger.h>
#include <uuid.h>

/* Ring Buffer Control Page (must match kernel definition) */
typedef struct RingControl {
	u32int magic;			/* 0x52494E47 "RING" */
	u32int version;			/* 1 */

	/* Submission Ring (User → Kernel) */
	struct {
		u32int head;		/* Consumer index (kernel) */
		u32int tail;		/* Producer index (user) */
		u32int mask;		/* Ring size - 1 */
		u32int flags;
		uuid_t uuids[120];	/* 120 UUIDv8 entries */
	} submission;

	/* Completion Ring (Kernel → User) */
	struct {
		u32int head;		/* Consumer index (user) */
		u32int tail;		/* Producer index (kernel) */
		u32int mask;
		u32int flags;
		uuid_t uuids[120];	/* 120 UUIDv8 entries */
	} completion;

	/* Security */
	uuid_t session_uuid;		/* UUIDv8 session identifier */

	/* Padding to 4KB */
	u8int reserved[200];
} RingControl;

/* Exchange Pool Handle */
typedef struct ExchPool {
	int chan_id;			/* Channel ID from #X/clone */
	int pool_fd;			/* File descriptor for #X/N/pool */
	int ctl_fd;			/* File descriptor for #X/N/ctl */
	RingControl *ring;		/* Mapped ring buffer control page */

	/* Pool configuration */
	int pool_size;			/* Number of pages in pool */

	/* Statistics */
	u64int submits;			/* Total submissions */
	u64int completions;		/* Total completions */
} ExchPool;

/* Exchange Pool API */

/* Initialize exchange pool and attach to kernel endpoint
 * Returns: ExchPool* on success, nil on failure
 */
ExchPool* exch_pool_init(int pool_size);

/* Allocate exchange page from pool
 * Returns: 0 on success, -1 on failure
 * Fills out_cap with the UserCapability for the allocated page
 */
int exch_alloc(ExchPool *pool, UserCapability *out_cap);

/* Map exchange page by capability
 * Returns: Virtual address on success, 0 on failure
 */
void* exch_map(const UserCapability *cap);

/* Unmap exchange page
 * Returns: 0 on success, -1 on failure
 */
int exch_unmap(void *addr, ulong size);

/* Submit exchange page UUID to ring buffer
 * Does NOT trigger doorbell - use exch_doorbell() for that
 * Returns: 0 on success, -1 on failure
 */
int exch_submit(ExchPool *pool, const uuid_t *page_uuid);

/* Ring the doorbell to notify kernel
 * This triggers syscall processing for all submitted UUIDs
 */
void exch_doorbell(void);

/* Wait for completion UUID from ring buffer
 * Returns: 0 on success, -1 on failure
 * Fills out_uuid with the completed UUID
 */
int exch_wait(ExchPool *pool, uuid_t *out_uuid);

/* Free exchange page back to pool
 * Returns: 0 on success, -1 on failure
 */
int exch_free(ExchPool *pool, const UserCapability *cap);

/* Cleanup and destroy exchange pool
 */
void exch_pool_destroy(ExchPool *pool);

#endif
