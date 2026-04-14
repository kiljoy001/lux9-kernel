/*
 * Minimal in-kernel /dev/ram backing store for the router-facing ABI.
 *
 * The historical devram device was removed, but the active 9P router still
 * exposes /dev/ram via flat read/write helpers. Keep that surface live with a
 * simple lazy-allocated ramdisk instead of failing every request.
 */
#include "u.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include <error.h>

enum {
  RamdiskSize = 8 * 1024 * 1024,
};

static uchar *ramdisk_data;
static ulong ramdisk_size = RamdiskSize;
static Lock ramdisk_lock;

static uchar *
ramdisk_get(void)
{
  uchar *data;

  data = ramdisk_data;
  if(data != nil)
    return data;

  lock(&ramdisk_lock);
  data = ramdisk_data;
  if(data == nil){
    data = xalloc(ramdisk_size);
    if(data != nil)
      ramdisk_data = data;
  }
  unlock(&ramdisk_lock);

  if(data == nil)
    error(Enomem);
  return data;
}

long
ramread(Chan *c, void *va, long n, vlong off)
{
  ulong count, uoff;
  uchar *data;

  USED(c);

  if(n < 0 || off < 0)
    error(Ebadarg);
  if(n == 0)
    return 0;
  if(va == nil)
    error(Ebadarg);

  uoff = (ulong)off;
  if(uoff >= ramdisk_size)
    return 0;

  count = (ulong)n;
  if(count > ramdisk_size - uoff)
    count = ramdisk_size - uoff;

  data = ramdisk_get();
  memmove(va, data + uoff, count);
  return (long)count;
}

long
ramwrite(Chan *c, void *va, long n, vlong off)
{
  ulong count, uoff;
  uchar *data;

  USED(c);

  if(n < 0 || off < 0)
    error(Ebadarg);
  if(n == 0)
    return 0;
  if(va == nil)
    error(Ebadarg);

  uoff = (ulong)off;
  if(uoff >= ramdisk_size)
    error(Eio);

  count = (ulong)n;
  if(count > ramdisk_size - uoff)
    count = ramdisk_size - uoff;

  data = ramdisk_get();
  memmove(data + uoff, va, count);
  return (long)count;
}

/*
 * Router-facing helpers use a flat buffer signature rather than Chan-based
 * device methods. Keep the ABI explicit so the 9P path can advertise the
 * actual backing size.
 */
long
ram9pread(void *va, long n, vlong off)
{
  return ramread(nil, va, n, off);
}

long
ram9pwrite(void *va, long n, vlong off)
{
  return ramwrite(nil, va, n, off);
}

ulong
ram9psize(void)
{
  return ramdisk_size;
}
