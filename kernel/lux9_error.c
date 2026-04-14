#include "fns.h"
#include "portdat.h"
#include "portlib.h"
#include "u.h"

/*
 * lux9_error - Kernel Error Handler
 *
 * Replaces the standard Plan 9 error() function (via macro in fns.h).
 * Copies the error string to the current process's error buffer and
 * triggers a non-local goto via nexterror().
 */
void lux9_error(char *e) {
  if (up == nil)
    panic("lux9_error: nil up: %s", e);

  kstrcpy(up->errstr, e, ERRMAX);
  nexterror();
}
