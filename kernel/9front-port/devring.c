#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "ipc_ring.h"
#include "borrowchecker.h"
#include "pageown.h"
#include "pebble.h"
#include "hhdm.h"

/*
 * Ring Buffer Device Driver (#R)
 * Implements "Page Flipping" IPC.
 */

enum {
    Qdir,
    Qctl,
    Qring
};

static Dirtab ringdir[] = {
    ".",    {Qdir, 0, QTDIR},   0,  0555,
    "ctl",  {Qctl, 0, QTFILE},  0,  0666,
    "0",    {Qring, 0, QTFILE}, 0,  0600,
};

/* Channel State */
struct ChannelState {
    Ref ref;
    struct IpcChannel *kmap_addr; /* Kernel mapping of Control Page */
    uintptr phys_addr;            /* Physical address of Control Page */
    Proc *owner;
    u64int last_seqno;            /* Last seen sequence number for replay protection */
    u64int session_id;            /* Random session ID to prevent cross-session attacks */
};

static struct ChannelState *channels[1];

static void
ringinit(void)
{
    channels[0] = nil;
}

static Chan*
ringattach(char *spec)
{
    return devattach('R', spec);
}

static Walkqid*
ringwalk(Chan *c, Chan *nc, char **name, int nname)
{
    return devwalk(c, nc, name, nname, ringdir, nelem(ringdir), devgen);
}

static int
ringstat(Chan *c, uchar *db, int n)
{
    return devstat(c, db, n, ringdir, nelem(ringdir), devgen);
}

static Chan*
ringopen(Chan *c, int omode)
{
    return devopen(c, omode, ringdir, nelem(ringdir), devgen);
}

static void
ringclose(Chan *c)
{
    if (c->aux) {
        // Cleanup logic
    }
}

static long
ringread(Chan *c, void *va, long n, vlong offset)
{
    if ((ulong)c->qid.path == Qctl)
        return readstr(offset, va, n, "ring 0: page-flip mode active\n");
    return devdirread(c, va, n, ringdir, nelem(ringdir), devgen);
}

static long
ringwrite(Chan *c, void *va, long n, vlong offset)
{
    return n;
}

/* mmap: Map the Control Page */
static void*
ringmmap(Chan *c, void *addr, long len, ulong offset)
{
    struct ChannelState *cs;
    
    if (channels[0] == nil) {
        void *p;
        UserCapability cap;
        
        if (pebble_alloc_with_white(4096, &cap, &p) < 0)
            error(Enomem);
            
        cs = xalloc(sizeof(struct ChannelState));
        cs->kmap_addr = p;
        cs->owner = up;
        cs->last_seqno = 0;  /* Initialize sequence number tracking */

        /* Generate random session ID for this channel */
        /* Uses RDRAND if available, falls back to TSC-based entropy */
        if (hwrandbuf != nil) {
            (*hwrandbuf)(&cs->session_id, sizeof(cs->session_id));
        } else {
            /* Fallback: combine TSC with process address for uniqueness */
            cs->session_id = rdtsc() ^ (u64int)(uintptr)up;
        }

        /* Init Control Page */
        memset(p, 0, 4096);
        cs->kmap_addr->magic = 0x52494E47;
        cs->kmap_addr->submission.mask = RING_MASK;
        cs->kmap_addr->completion.mask = RING_MASK;

        channels[0] = cs;
    }
    return channels[0]->kmap_addr;
}

/*
 * Process a single 9P message from the batch.
 */
static void
process_message(u8int *data, int len)
{
    /* 
     * Real implementation would dispatch to 9P server logic.
     * For now, just debug print.
     */
    // print("Ring Msg: len=%d\n", len);
}

/*
 * The Kernel Consumer Loop
 * 1. Checks Submission Ring.
 * 2. Acquires Page (Batch).
 * 3. Iterates Messages.
 * 4. Returns Page to Completion Ring.
 */
static void
ring_process_batch(struct ChannelState *cs)
{
    struct IpcChannel *chan = cs->kmap_addr;
    u32int head, tail;
    u64int page_handle;
    uintptr page_phys, user_vaddr;
    struct BatchHeader *batch;
    int i, offset;
    u64int *pte;

    head = chan->submission.head;
    tail = chan->submission.tail;

    while (head != tail) {
        /* 1. Get Page Handle (User VA) */
        page_handle = chan->submission.pages[head & RING_MASK];
        user_vaddr = (uintptr)page_handle;

        /* 2. SECURITY: Verify & Acquire (The "Flip") */

        /* Validate alignment */
        if ((user_vaddr & (BY2PG-1)) != 0) {
            print("ring: invalid page alignment: %#p\n", user_vaddr);
            goto skip_page;
        }

        /* Translate user VA to physical address using current MMU context
         * Note: The submitting process must be the current process (cs->owner == up)
         * for this to work correctly. The ring buffer is per-process.
         */
        if (cs->owner != up) {
            print("ring: process mismatch: owner %p != up %p\n", cs->owner, up);
            goto skip_page;
        }

        pte = mmuwalk(m->pml4, user_vaddr, 0, 0);
        if (pte == nil || (*pte & PTEVALID) == 0) {
            print("ring: invalid page mapping: %#p\n", user_vaddr);
            goto skip_page;
        }

        page_phys = PADDR(*pte);

        /* SECURITY: Verify page ownership via borrow checker */
        if (!pageown_is_owned(page_phys)) {
            print("ring: page not owned: pa=%#p\n", page_phys);
            goto skip_page;
        }

        if (pageown_get_owner(page_phys) != cs->owner) {
            print("ring: page owned by different process: pa=%#p\n", page_phys);
            goto skip_page;
        }

        /* SECURITY: Verify page can be borrowed (no active mut borrows) */
        if (!pageown_can_borrow_shared(page_phys)) {
            print("ring: page has active mutable borrow: pa=%#p\n", page_phys);
            goto skip_page;
        }

        /* ANTI-TOCTOU: Unmap page from user space BEFORE accessing it
         * This prevents user from modifying the page while kernel processes it.
         * Critical for preventing TOCTOU attacks on batch header validation.
         */
        u64int saved_pte = *pte;
        *pte = 0;  /* Atomically remove user access */
        putcr3(getcr3());  /* Flush TLB - now user cannot access page */

        /* Map to kernel via HHDM - only kernel can access now */
        batch = (struct BatchHeader*)hhdm_virt(page_phys);

        /* SECURITY: Validate batch header
         * ANTI-TOCTOU: Copy header fields to stack to prevent user modification
         * during validation. User can no longer access page (unmapped above).
         */
        u32int batch_magic = batch->magic;
        u16int batch_num_messages = batch->num_messages;
        u16int batch_used_bytes = batch->used_bytes;
        u64int batch_seqno = batch->nonce;

        if (batch_magic != BATCH_PAGE_MAGIC) {
            print("ring: invalid batch magic: %#ux (expected %#ux)\n",
                  batch_magic, BATCH_PAGE_MAGIC);
            goto restore_page;
        }

        /* SECURITY: Bounds check num_messages */
        if (batch_num_messages > 256) {
            print("ring: too many messages: %ud (max 256)\n", batch_num_messages);
            goto restore_page;
        }

        /* SECURITY: Validate used_bytes */
        if (batch_used_bytes < BATCH_DATA_START || batch_used_bytes > 4096) {
            print("ring: invalid used_bytes: %ud\n", batch_used_bytes);
            goto restore_page;
        }

        /* SECURITY: Validate sequence number for replay protection */
        /* The nonce field acts as a monotonic sequence number, NOT a cryptographic nonce */
        if (batch_seqno <= cs->last_seqno) {
            print("ring: replay detected: seqno %llud <= last %llud\n",
                  batch_seqno, cs->last_seqno);
            goto restore_page;
        }
        cs->last_seqno = batch_seqno;

        /* 3. Process Batch
         * ANTI-TOCTOU: Use copied values (batch_num_messages, batch_used_bytes)
         * from stack, not from batch page. User cannot modify them anymore.
         */
        offset = BATCH_DATA_START;
        for (i = 0; i < batch_num_messages; i++) {
            /* Check we can read message length */
            if (offset + 2 > batch_used_bytes) {
                print("ring: message header exceeds used_bytes at offset %d\n", offset);
                break;
            }

            /* ANTI-TOCTOU: Copy message length to stack before validation
             * Prevents user from modifying msg_len between check and use
             */
            u16int msg_len = *(u16int*)((u8int*)batch + offset);
            offset += 2;

            /* SECURITY: Validate message length */
            if (msg_len > 8192) {
                print("ring: message too large: %ud bytes\n", msg_len);
                break;
            }

            /* Check message data fits in batch */
            if (offset + msg_len > batch_used_bytes) {
                print("ring: message data exceeds used_bytes\n");
                break;
            }

            /* Check no integer overflow */
            if (offset + msg_len < offset) {
                print("ring: integer overflow in message bounds\n");
                break;
            }

            process_message((u8int*)batch + offset, msg_len);
            offset += msg_len;
        }

restore_page:
        /* ANTI-TOCTOU: Restore user access to page after processing
         * Re-map the page back into user space so they can reuse it
         */
        *pte = saved_pte;
        putcr3(getcr3());  /* Flush TLB to restore user access */

skip_page:
        /* 4. Return to Completion Ring */
        u32int c_tail = chan->completion.tail;
        chan->completion.pages[c_tail & RING_MASK] = page_handle;
        chan->completion.tail++;

        head++;
    }
    chan->submission.head = head;
}

Dev ringdevtab = {
    'R',
    "ring",

    devreset,
    ringinit,
    devshutdown,
    ringattach,
    ringwalk,
    ringstat,
    ringopen,
    devcreate,
    ringclose,
    ringread,
    devbread,
    ringwrite,
    devbwrite,
    devremove,
    devwstat,
};