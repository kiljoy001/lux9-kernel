#include <libc.h>
#include <u.h>

/*@
  @ requires n >= 0;
  @ requires \valid_read(s1 + (0 .. n-1)) || (\exists integer k; 0 <= k < n &&
  s1[k] == '\0');
  @ requires \valid_read(s2 + (0 .. n-1)) || (\exists integer k; 0 <= k < n &&
  s2[k] == '\0');
  @ assigns \nothing;
  @ ensures \result == -1 || \result == 0 || \result == 1;
  @*/
int cistrncmp(char *s1, char *s2, int n) {
  int c1, c2;

  while (n > 0) {
    c1 = *(unsigned char *)s1++;
    c2 = *(unsigned char *)s2++;
    n--;
    if (c1 >= 'A' && c1 <= 'Z')
      c1 += 'a' - 'A';
    if (c2 >= 'A' && c2 <= 'Z')
      c2 += 'a' - 'A';
    if (c1 != c2) {
      if (c1 > c2)
        return 1;
      return -1;
    }
    if (c1 == 0)
      break;
  }
  return 0;
}
