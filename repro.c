typedef unsigned int Rune;
typedef struct Quoteinfo Quoteinfo;
struct Quoteinfo {
  int quoted;
  int nrunesin;
  int nbytesin;
  int nrunesout;
  int nbytesout;
};
extern int runelen(int);
extern int chartorune(Rune *, char *);
extern int (*doquote)(int);

void _quotesetup(char *s, Rune *r, int nin, int nout, Quoteinfo *q, int sharp,
                 int runesout) {
  int w;
  Rune c;

  q->quoted = 0;
  q->nbytesout = 0;
  if (sharp || nin == 0 || (s && *s == '\0') || (r && *r == '\0')) {
    if (nout < 2)
      return;
    q->quoted = 1;
    q->nbytesout = 2;
  }
}
