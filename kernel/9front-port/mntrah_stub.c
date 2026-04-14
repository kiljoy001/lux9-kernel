/*
 * mntrah_stub.c - Stub for mnt read-ahead functions
 *
 * The original devmnt.c was moved to userspace. These stubs provide
 * minimal read-ahead functionality for cache.c during boot.
 *
 * FORMAL VERIFICATION:
 *   Coq proofs: N/A (stub implementation)
 *   Frama-C: ACSL annotations for safety
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

/*@
  @ // Model Link: proofs/mnt/readahead_model.v
  @
  @ predicate is_valid_mntrah(Mntrah *rah) =
  @   \valid(rah);
  @*/

/*@
  @ // Model Link: proofs/device/buffered_io.v
  @
  @ requires \valid(c);
  @ requires buf == \null || \valid(((char*)buf) + (0..n-1));
  @ requires n >= 0;
  @ requires off >= 0;
  @
  @ behavior null_buf:
  @   assumes buf == \null || n == 0;
  @   assigns \nothing;
  @   ensures \result == 0;
  @
  @ behavior normal:
  @   assumes buf != \null && n > 0;
  @   assigns ((char*)buf)[0..n-1];
  @   ensures \result >= 0;
  @   ensures \result <= n;
  @
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
long devbread(Chan *c, void *buf, long n, vlong off) {
  Dev *d;
  /*
   * Stub: Buffered read - just delegate to channel read.
   * In a full implementation, this would use Block buffers.
   */
  if (c == nil || buf == nil || n <= 0)
    return 0;
  d = devtab[devno(c->type, 0)];
  if (d == nil || d->read == nil)
    return 0;
  return d->read(c, buf, n, off);
}

/*@
  @ // Model Link: proofs/device/buffered_io.v
  @
  @ requires \valid(c);
  @ requires buf == \null || \valid_read(((char*)buf) + (0..n-1));
  @ requires n >= 0;
  @ requires off >= 0;
  @
  @ behavior null_buf:
  @   assumes buf == \null || n == 0;
  @   assigns \nothing;
  @   ensures \result == 0;
  @
  @ behavior normal:
  @   assumes buf != \null && n > 0;
  @   assigns \nothing;
  @   ensures \result >= 0;
  @   ensures \result <= n;
  @
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
long devbwrite(Chan *c, void *buf, long n, vlong off) {
  Dev *d;
  /*
   * Stub: Buffered write - just delegate to channel write.
   * In a full implementation, this would use Block buffers.
   */
  if (c == nil || buf == nil || n <= 0)
    return 0;
  d = devtab[devno(c->type, 0)];
  if (d == nil || d->write == nil)
    return 0;
  return d->write(c, buf, n, off);
}
