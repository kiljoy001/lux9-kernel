#include "fns.h"
#include "mem.h"
#include "portdat.h"
#include "portlib.h"
#include "u.h"

/*
 * Kernel Configuration Environment (kconf)
 * Handles boot parameters and global configuration.
 * Replaces the 'confegrp' logic from devenv.c.
 */

typedef struct KConfEntry KConfEntry;
struct KConfEntry {
  KConfEntry *next;
  char *name;
  char *value;
};

static struct {
  Lock lock;
  KConfEntry *head;
} kconf;

/*
 * Set a kernel configuration variable.
 * Safe to call during boot.
 */
void kconf_set(char *name, char *val) {
  KConfEntry *e;
  int n_name, n_val;

  lock(&kconf.lock);

  /* Update existing */
  for (e = kconf.head; e != nil; e = e->next) {
    if (strcmp(e->name, name) == 0) {
      free(e->value);
      n_val = strlen(val) + 1;
      e->value = malloc(n_val);
      if (e->value == nil)
        panic("kconf_set: malloc value");
      memmove(e->value, val, n_val);
      unlock(&kconf.lock);
      return;
    }
  }

  /* Add new */
  e = malloc(sizeof(KConfEntry));
  if (e == nil)
    panic("kconf_set: malloc entry");

  n_name = strlen(name) + 1;
  e->name = malloc(n_name);
  if (e->name == nil)
    panic("kconf_set: malloc name");
  memmove(e->name, name, n_name);

  n_val = strlen(val) + 1;
  e->value = malloc(n_val);
  if (e->value == nil)
    panic("kconf_set: malloc value");
  memmove(e->value, val, n_val);

  e->next = kconf.head;
  kconf.head = e;

  unlock(&kconf.lock);
}

/*
 * Wrapper for legacy ksetenv calls.
 * Ignores 'conf' flag - everything during boot goes to kconf.
 */
void ksetenv(char *name, char *val, int conf) {
  /* We ignore 'conf' because without #e, all boot-time
     invocations are treated as kernel config/boot params */
  kconf_set(name, val);
}

/*
 * Return a copy of configuration environment as a sequence of strings.
 * The strings alternate between name and value. A zero length name string
 * indicates the end of the list.
 */
char *getconfenv(void) {
  KConfEntry *e;
  char *p, *q;
  ulong n;

  lock(&kconf.lock);

  /* Calculate size */
  n = 1; /* Terminating null */
  for (e = kconf.head; e != nil; e = e->next) {
    n += strlen(e->name) + strlen(e->value) + 2; /* name \0 value \0 */
  }

  p = malloc(n);
  if (p == nil) {
    unlock(&kconf.lock);
    return nil;
  }

  q = p;
  for (e = kconf.head; e != nil; e = e->next) {
    int len;

    len = strlen(e->name) + 1;
    memmove(q, e->name, len);
    q += len;

    len = strlen(e->value) + 1;
    memmove(q, e->value, len);
    q += len;
  }
  *q = '\0';

  unlock(&kconf.lock);
  return p;
}
