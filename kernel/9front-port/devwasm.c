#include "u.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "../wasm/wasm_runtime.h"

enum { Qdir, Qctl };

static Dirtab wasmdir[] = {
    ".", {Qdir, 0, QTDIR}, 0, 0555, "ctl", {Qctl, 0, QTFILE}, 0, 0666,
};

static Chan *wasmattach(char *spec) { return devattach('W', spec); }

static Walkqid *wasmwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, wasmdir, nelem(wasmdir), devgen);
}

static int wasmstat(Chan *c, uchar *db, int n) {
  return devstat(c, db, n, wasmdir, nelem(wasmdir), devgen);
}

static Chan *wasmopen(Chan *c, int omode) {
  return devopen(c, omode, wasmdir, nelem(wasmdir), devgen);
}

static void wasmclose(Chan *) {}

static long wasmread(Chan *c, void *va, long n, vlong offset) {
  if ((ulong)c->qid.path == Qctl)
    return readstr(offset, va, n, "stats: dump wasm runtime stats\n");
  return devdirread(c, va, n, wasmdir, nelem(wasmdir), devgen);
}

static long wasmwrite(Chan *c, void *va, long n, vlong) {
  char buf[32];

  if ((ulong)c->qid.path != Qctl)
    error(Eperm);
  if (n <= 0)
    return 0;

  if (n >= (long)sizeof(buf))
    n = sizeof(buf) - 1;
  memmove(buf, va, n);
  buf[n] = 0;

  if (strncmp(buf, "stats", 5) == 0) {
    wasm_runtime_stats();
    return n;
  }

  error("bad wasm ctl");
  return 0;
}

Dev wasmdevtab = {
    .dc = 'W',
    .name = "wasm",
    .attach = wasmattach,
    .walk = wasmwalk,
    .stat = wasmstat,
    .open = wasmopen,
    .close = wasmclose,
    .read = wasmread,
    .write = wasmwrite,
};
