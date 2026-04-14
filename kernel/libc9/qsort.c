#include <libc.h>
#include <u.h>

static void swap(char *a, char *b, ulong es) {
  char c;

  while (es > 0) {
    c = *a;
    *a++ = *b;
    *b++ = c;
    es--;
  }
}

void qsort(void *a, ulong n, ulong es, int (*cmp)(const void *, const void *)) {
  char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
  int r, swaptype;

  if (n < 7) { /* Insertion sort on smallest arrays */
    for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
      for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0; pl -= es)
        swap(pl, pl - es, es);
    return;
  }

  pm = (char *)a + (n / 2) * es;
  if (n > 7) {
    pl = a;
    pn = (char *)a + (n - 1) * es;
    if (n > 40) {
      long d = (n / 8) * es;
      pl = (char *)
          a; /* Median of 3 for median of medians? No, just pseudo-median */
      /* Actually, standard BSD qsort uses median of 9 for large arrays, but
       * let's stick to simple median of 3 */
    }
    /* Median of 3 */
    if (cmp(pl, pm) > 0)
      swap(pl, pm, es);
    if (cmp(pl, pn) > 0)
      swap(pl, pn, es);
    if (cmp(pm, pn) > 0)
      swap(pm, pn, es);
  }
  swap(a, pm, es);
  pa = pb = (char *)a + es;
  pc = pd = (char *)a + (n - 1) * es;
  for (;;) {
    while (pb <= pc && (r = cmp(pb, a)) <= 0) {
      if (r == 0) {
        swap(pa, pb, es);
        pa += es;
      }
      pb += es;
    }
    while (pb <= pc && (r = cmp(pc, a)) >= 0) {
      if (r == 0) {
        swap(pc, pd, es);
        pd -= es;
      }
      pc -= es;
    }
    if (pb > pc)
      break;
    swap(pb, pc, es);
    pb += es;
    pc -= es;
  }
  pn = (char *)a + n * es;
  long r1 = pa - (char *)a;
  long r2 = pb - pa;
  if (r2 < r1)
    r1 = r2;
  swap(a, pb - r1, r1);

  r1 = pd - pc;
  r2 = pn - pd - es;
  if (r2 < r1)
    r1 = r2;
  swap(pb, pn - r1, r1);

  if ((r1 = pb - pa) > es)
    qsort(a, r1 / es, es, cmp);
  if ((r1 = pd - pc) > es)
    qsort(pn - r1, r1 / es, es, cmp);
}
