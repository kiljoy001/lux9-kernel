#include <u.h>
#include <libc.h>
#include "fmtdef.h"

/*
 * How many bytes of output UTF will be produced by quoting (if necessary) this string?
 * How many runes? How much of the input will be consumed?
 * The parameter q is filled in by _quotesetup.
 * The string may be UTF or Runes (s or r).
 * Return count does not include NUL.
 * Terminate the scan at the first of:
 *	NUL in input
 *	count exceeded in input
 *	count exceeded on output
 * *ninp is set to number of input bytes accepted.
 * nin may be <0 initially, to avoid checking input by count.
 */
/*@
  @ requires \valid(q);
  @ requires s != \null ==> \valid_read(s + (0..));
  @ requires r != \null ==> \valid_read(r + (0..));
  @ assigns *q;
  @ ensures q->nbytesout >= 0;
  @ ensures q->nrunesout >= 0;
  @ ensures q->nbytesin >= 0;
  @ ensures q->nrunesin >= 0;
  @*/
void
_quotesetup(char *s, Rune *r, int nin, int nout, Quoteinfo *q, int sharp, int runesout)
{
	int w;
	Rune c;

	q->quoted = 0;
	q->nbytesout = 0;
	q->nrunesout = 0;
	q->nbytesin = 0;
	q->nrunesin = 0;
	if(sharp || nin==0 || (s && *s=='\0') || (r && *r=='\0')){
		if(nout < 2)
			return;
		q->quoted = 1;
		q->nbytesout = 2;
		q->nrunesout = 2;
	}
	for(; nin!=0; nin--){
		if(s)
			w = chartorune(&c, s);
		else{
			c = *r;
			w = runelen(c);
		}

		if(c == '\0')
			break;
		if(runesout){
			if(q->nrunesout+1 > nout)
				break;
		}else{
			if(q->nbytesout+w > nout)
				break;
		}

		if((c <= L' ') || (c == L'\'') || (doquote!=nil && doquote(c))){
			if(!q->quoted){
				if(runesout){
					if(1+q->nrunesout+1+1 > nout)	/* no room for quotes */
						break;
				}else{
					if(1+q->nbytesout+w+1 > nout)	/* no room for quotes */
						break;
				}
				q->nrunesout += 2;	/* include quotes */
				q->nbytesout += 2;	/* include quotes */
				q->quoted = 1;
			}
			if(c == '\'')	{
				if(runesout){
					if(1+q->nrunesout+1 > nout)	/* no room for quotes */
						break;
				}else{
					if(1+q->nbytesout+w > nout)	/* no room for quotes */
						break;
				}
				q->nbytesout++;
				q->nrunesout++;	/* quotes reproduce as two characters */
			}
		}

		/* advance input */
		if(s)
			s += w;
		else
			r++;
		q->nbytesin += w;
		q->nrunesin++;

		/* advance output */
		q->nbytesout += w;
		q->nrunesout++;
	}
}

/*@
  @ requires \\valid_read(q);
  @ requires \\valid(f);
  @ requires sin != \\null ==> \\valid_read(sin + (0 .. q->nbytesin-1));
  @ requires rin != \\null ==> \\valid_read(rin + (0 .. q->nrunesin-1));
  @ requires q->nbytesin >= 0;
  @ requires q->nrunesin >= 0;
  @ requires q->nbytesout >= 0;
  @ requires q->nrunesout >= 0;
  @ assigns *f;
  @ ensures \\result == 0 || \\result == -1;
  @*/
static int
qstrfmt(char *sin, Rune *rin, Quoteinfo *q, Fmt *f)
{
	Rune r, *rm, *rme;
	char *t, *s, *m, *me;
	Rune *rt, *rs;
	ulong fl;
	int nc, w;

	m = sin;
	me = m + q->nbytesin;
	rm = rin;
	rme = rm + q->nrunesin;

	w = f->width;
	fl = f->flags;
	if(f->runes){
		if(!(fl & FmtLeft) && _rfmtpad(f, w - q->nrunesout) < 0)
			return -1;
	}else{
		if(!(fl & FmtLeft) && _fmtpad(f, w - q->nbytesout) < 0)
			return -1;
	}
	t = f->to;
	s = f->stop;
	rt = f->to;
	rs = f->stop;
	if(f->runes)
		FMTRCHAR(f, rt, rs, '\'');
	else
		FMTRUNE(f, t, s, '\'');
	for(nc = q->nrunesin; nc > 0; nc--){
		if(sin){
			r = *(uchar*)m;
			if(r < Runeself)
				m++;
			else if((me - m) >= UTFmax || fullrune(m, me-m))
				m += chartorune(&r, m);
			else
				break;
		}else{
			if(rm >= rme)
				break;
			r = *(uchar*)rm++;
		}
		if(f->runes){
			FMTRCHAR(f, rt, rs, r);
			if(r == '\'')
				FMTRCHAR(f, rt, rs, r);
		}else{
			FMTRUNE(f, t, s, r);
			if(r == '\'')
				FMTRUNE(f, t, s, r);
		}
	}

	if(f->runes){
		FMTRCHAR(f, rt, rs, '\'');
		USED(rs);
		f->nfmt += rt - (Rune *)f->to;
		f->to = rt;
		if(fl & FmtLeft && _rfmtpad(f, w - q->nrunesout) < 0)
			return -1;
	}else{
		FMTRUNE(f, t, s, '\'');
		USED(s);
		f->nfmt += t - (char *)f->to;
		f->to = t;
		if(fl & FmtLeft && _fmtpad(f, w - q->nbytesout) < 0)
			return -1;
	}
	return 0;
}

/*@
  @ requires \\valid(f);
  @ requires f->runes == 0 || f->runes == 1;
  @ assigns *f;
  @ ensures \\result == 0 || \\result == -1;
  @*/
int
_quotestrfmt(int runesin, Fmt *f)
{
	int nin, outlen;
	Rune *r;
	char *s;
	Quoteinfo q;

	nin = -1;
	if(f->flags&FmtPrec)
		nin = f->prec;
	if(runesin){
		r = va_arg(f->args, Rune *);
		s = nil;
	}else{
		s = va_arg(f->args, char *);
		r = nil;
	}
	if(!s && !r)
		return _fmtcpy(f, "<nil>", 5, 5);

	if(f->flush)
		outlen = 0x7FFFFFFF;	/* if we can flush, no output limit */
	else if(f->runes)
		outlen = (Rune*)f->stop - (Rune*)f->to;
	else
		outlen = (char*)f->stop - (char*)f->to;

	_quotesetup(s, r, nin, outlen, &q, f->flags&FmtSharp, f->runes);
//print("bytes in %d bytes out %d runes in %d runesout %d\n", q.nbytesin, q.nbytesout, q.nrunesin, q.nrunesout);

	if(runesin){
		if(!q.quoted)
			return _fmtrcpy(f, r, q.nrunesin);
		return qstrfmt(nil, r, &q, f);
	}

	if(!q.quoted)
		return _fmtcpy(f, s, q.nrunesin, q.nbytesin);
	return qstrfmt(s, nil, &q, f);
}

/*@
  @ requires \\valid(f);
  @ assigns *f;
  @ ensures \\result == 0 || \\result == -1;
  @*/
int
quotestrfmt(Fmt *f)
{
	return _quotestrfmt(0, f);
}

/*@
  @ requires \\valid(f);
  @ assigns *f;
  @ ensures \\result == 0 || \\result == -1;
  @*/
int
quoterunestrfmt(Fmt *f)
{
	return _quotestrfmt(1, f);
}

/*@
  @ assigns \\nothing;
  @*/
void
quotefmtinstall(void)
{
	fmtinstall('q', quotestrfmt);
	fmtinstall('Q', quoterunestrfmt);
}

/*@
  @ requires \\valid_read(s + (0..));
  @ requires \\valid(quotelenp);
  @ requires \\exists integer n; n >= 0 && s[n] == '\\0';
  @ assigns *quotelenp;
  @ ensures \\result == 0 || \\result == 1;
  @ ensures *quotelenp >= 0;
  @*/
int
_needsquotes(char *s, int *quotelenp)
{
	Quoteinfo q;

	_quotesetup(s, nil, -1, 0x7FFFFFFF, &q, 0, 0);
	*quotelenp = q.nbytesout;

	return q.quoted;
}

/*@
  @ requires \\valid_read(r + (0..));
  @ requires \\valid(quotelenp);
  @ requires \\exists integer n; n >= 0 && r[n] == 0;
  @ assigns *quotelenp;
  @ ensures \\result == 0 || \\result == 1;
  @ ensures *quotelenp >= 0;
  @*/
int
_runeneedsquotes(Rune *r, int *quotelenp)
{
	Quoteinfo q;

	_quotesetup(nil, r, -1, 0x7FFFFFFF, &q, 0, 0);
	*quotelenp = q.nrunesout;

	return q.quoted;
}
