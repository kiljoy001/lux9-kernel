#include <u.h>
#include <portlib.h>

/*@
  @ requires \valid_read(s + (0..));
  @ requires \exists integer n; n >= 0 && s[n] == '\0';
  @ assigns \nothing;
  @ ensures \result == strtol(s, nil, 10);
  @*/
long
atol(char *s)
{
	return strtol(s, nil, 10);
}

/*@
  @ requires \valid_read(s + (0..));
  @ requires \exists integer n; n >= 0 && s[n] == '\0';
  @ assigns \nothing;
  @ ensures \result == (int)strtol(s, nil, 10);
  @*/
int
atoi(char *s)
{
	return strtol(s, nil, 10);
}
