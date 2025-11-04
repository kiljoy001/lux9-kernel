/*
 * Unified borrow checker for kernel primitives
 * Provides Rust-style ownership and borrowing for locks, memory, I/O, etc.
 * 
 * Enhanced with system-level ownership for boot memory coordination
 * between bootloader, kernel, and HHDM memory systems.
 */

#pragma once

/* System-level ownership types for boot coordination */
enum BorrowSystemOwner {
	OWNER_BOOTLOADER = 0,    /* Limine bootloader owns this memory */
	OWNER_KERNEL,            /* 9front kernel owns this memory */
};

/* Borrow states - based on Rust borrow semantics */
enum BorrowState {
	BORROW_FREE = 0,          /* Resource is unowned */
	BORROW_EXCLUSIVE,         /* Owned exclusively by one process */
	BORROW_SHARED_OWNED,      /* Owner has resource, but lent as shared */
	BORROW_MUT_LENT,          /* Owner lent resource as mutable, blocked */
};

/* Shared borrower tracking - linked list node */
struct SharedBorrower {
	Proc	*proc;			/* Process that has a shared borrow */
	struct SharedBorrower *next;	/* Next shared borrower */
};

/* Per-resource ownership tracking descriptor */
struct BorrowOwner {
	/* Resource identification */
	uintptr	key;			/* Unique key for the resource (e.g., address) */

	/* Core ownership */
	Proc	*owner;			/* Original owner process */
	enum BorrowState state;		/* Current ownership state */

	/* System-level ownership (for boot coordination) */
	enum BorrowSystemOwner system_owner;	/* System-level owner (bootloader/kernel/HHDM) */
	int is_system_owned;		/* True if owned by a system (not a process) */

	/* Borrow tracking */
	int	shared_count;		/* Number of shared borrows (&) */
	struct SharedBorrower *shared_list;	/* List of shared borrowers */
	Proc	*mut_borrower;		/* Exclusive mutable borrower (&mut) */

	/* Lifetime tracking */
	uvlong	acquired_ns;		/* When ownership was acquired */
	uvlong	borrow_deadline_ns;	/* Automatic return time (0 = never) */

	/* Debugging */
	ulong	borrow_count;		/* Total times borrowed */

	struct BorrowOwner *next;	/* For hash table chaining */
};

/* Hash table bucket for BorrowOwner */
struct BorrowBucket {
	struct BorrowOwner *head;
};

/* Borrow pool - hash table of resources */
struct BorrowPool {
	Lock lock;
	struct BorrowBucket *owners;	/* Hash table buckets */
	ulong	nbuckets;		/* Number of hash buckets */
	ulong	nowners;		/* Total number of owned resources */
	ulong	nshared;		/* Resources with shared borrows */
	ulong	nmut;			/* Resources with mutable borrows */
};

/* Error codes for borrow operations */
enum BorrowError {
	BORROW_OK = 0,
	BORROW_EALREADY,		/* Already owned */
	BORROW_ENOTOWNER,		/* Not the owner */
	BORROW_EBORROWED,		/* Can't modify - has borrows */
	BORROW_EMUTBORROW,		/* Can't borrow - already &mut */
	BORROW_ESHAREDBORROW,		/* Can't borrow &mut - has & */
	BORROW_ENOTBORROWED,		/* Not borrowed, can't return */
	BORROW_EINVAL,			/* Invalid parameters */
	BORROW_ENOMEM,			/* Out of memory */
	BORROW_ENOTFOUND,		/* Resource not found */
};

/* Global borrow pool */
extern struct BorrowPool borrowpool;

/* Core ownership operations */
void	borrowinit(void);
enum BorrowError borrow_acquire(Proc *p, uintptr key);
enum BorrowError borrow_release(Proc *p, uintptr key);
enum BorrowError borrow_transfer(Proc *from, Proc *to, uintptr key);

/* Borrow operations (Rust-style) */
enum BorrowError borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_return_shared(Proc *borrower, uintptr key);
enum BorrowError borrow_return_mut(Proc *borrower, uintptr key);

/* Query operations */
int	borrow_is_owned(uintptr key);
Proc*	borrow_get_owner(uintptr key);
int	borrow_get_owner_snapshot(uintptr key, struct BorrowOwner *out);
enum BorrowState borrow_get_state(uintptr key);
int	borrow_can_borrow_shared(uintptr key);
int	borrow_can_borrow_mut(uintptr key);

/* Process cleanup - called when process dies */
void	borrow_cleanup_process(Proc *p);

/* Statistics and debugging */
void	borrow_stats(void);
void	borrow_dump_resource(uintptr key);

/* Hash function for keys */
ulong	borrow_hash(uintptr key);

/* System-level ownership operations - NEW */
enum BorrowError borrow_acquire_system(uintptr key, enum BorrowSystemOwner owner);
enum BorrowError borrow_release_system(uintptr key, enum BorrowSystemOwner owner);
enum BorrowError borrow_transfer_system(enum BorrowSystemOwner from, enum BorrowSystemOwner to, uintptr key);

/* System ownership queries */
enum BorrowSystemOwner borrow_get_system_owner(uintptr key);
int borrow_is_owned_by_system(uintptr key, enum BorrowSystemOwner owner);

/* Range-based memory tracking (efficient for large regions) */
struct MemoryRange {
	uintptr start;
	uintptr end;
	enum BorrowSystemOwner owner;
	struct MemoryRange *next;
};

/* Range-based tracking operations */
void memory_range_init(void);
void memory_range_add(uintptr start, uintptr end, enum BorrowSystemOwner owner);
void memory_range_remove(uintptr start, uintptr end);
void memory_range_add_discovered(uintptr start, uintptr end, enum BorrowSystemOwner owner);  /* Runtime discovery */
enum BorrowSystemOwner memory_range_get_owner(uintptr addr);
int memory_range_check_access(uintptr addr, enum BorrowSystemOwner requester);
void memory_range_dump(void);  /* Debug: show all ranges */
int memory_range_capacity(void);  /* How many more ranges can we add? */

/* Per-page tracking (for fine-grained runtime tracking after boot) */
enum BorrowError borrow_acquire_range_phys(uintptr start_pa, usize size, enum BorrowSystemOwner owner);
int borrow_range_owned_by_system(uintptr start_pa, usize size, enum BorrowSystemOwner owner);
int borrow_can_access_range_phys(uintptr start_pa, usize size, enum BorrowSystemOwner requester);

/* Memory coordination functions - simplified for boot handoff */
void boot_memory_coordination_init(void);
void transfer_bootloader_to_kernel(void);
void establish_memory_ownership_zones(void);
void establish_memory_ownership_zones_dynamic(void);  /* Runtime discovery from conf.mem[] */
int validate_memory_coordination_ready(void);
int post_cr3_memory_system_operational(void);

/* Memory system state tracking - NEW */
enum MemorySystemState {
	MEMORY_BOOTLOADER,      /* Limine-only memory system */
	MEMORY_TRANSITIONING,   /* In process of handoff */
	MEMORY_KERNEL_ACTIVE,   /* 9front memory system active */
	MEMORY_COORDINATED,     /* Full coordination operational */
};

struct MemoryCoordination {
	enum MemorySystemState state;
	enum BorrowSystemOwner current_owner;
	int coordination_enabled;
};
