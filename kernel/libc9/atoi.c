#include "acsl_bounds.h"
#include <portlib.h>
#include <u.h>

/*@
  @ requires valid_string(s);
  @ requires \exists integer n; n >= 0 && s[n] == '\0';
  @ assigns \nothing;
  @ ensures \result == strtol(s, nil, 10);
  @*/
long atol(char *s) { return strtol(s, nil, 10); }

/*@
  @ requires valid_string(s);
  @ requires \exists integer n; n >= 0 && s[n] == '\0';
  @ assigns \nothing;
  @ ensures \result == (int)strtol(s, nil, 10);
  @*/
int atoi(const char *s) { return strtol(s, nil, 10); }
