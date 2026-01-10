#include <u.h>
#include <libc.h>

static char qsep[] = " \t\r\n";

/*@
  @ requires \valid(s + (0..));
  @ requires \valid_read(sep + (0..));
  @ requires \exists integer n1; n1 >= 0 && s[n1] == '\0';
  @ requires \exists integer n2; n2 >= 0 && sep[n2] == '\0';
  @ assigns s[0 .. \strlen(\at(s, Pre))];
  @ ensures \valid_read(\result + (0..));
  @ ensures \result >= \at(s, Pre);
  @ ensures \forall integer i; 0 <= i < (\result - \at(s, Pre)) ==> \at(s, Pre)[i] != '\0';
  @*/
static char*
qtoken(char *s, char *sep)
{
	int quoting;
	char *t;

	quoting = 0;
	t = s;	/* s is output string, t is input string */
	/*@
	  @ loop invariant t >= s >= \at(s, Pre);
	  @ loop invariant quoting == 0 || quoting == 1;
	  @ loop invariant \valid(s);
	  @ loop invariant \valid_read(t);
	  @ loop assigns s, t, quoting, \at(s, Pre)[0 .. \strlen(\at(s, Pre))];
	  @*/
	while(*t!='\0' && (quoting || utfrune(sep, *t)==nil)){
		if(*t != '\''){
			*s++ = *t++;
			continue;
		}
		/* *t is a quote */
		if(!quoting){
			quoting = 1;
			t++;
			continue;
		}
		/* quoting and we're on a quote */
		if(t[1] != '\''){
			/* end of quoted section; absorb closing quote */
			t++;
			quoting = 0;
			continue;
		}
		/* doubled quote; fold one quote into two */
		t++;
		*s++ = *t++;
	}
	if(*s != '\0'){
		*s = '\0';
		if(t == s)
			t++;
	}
	return t;
}

/*@
  @ requires \valid_read(t + (0..));
  @ requires \valid_read(sep + (0..));
  @ requires \exists integer n1; n1 >= 0 && t[n1] == '\0';
  @ requires \exists integer n2; n2 >= 0 && sep[n2] == '\0';
  @ assigns \nothing;
  @ ensures \result >= t;
  @ ensures \valid_read(\result);
  @*/
static char*
etoken(char *t, char *sep)
{
	int quoting;

	/* move to end of next token */
	quoting = 0;
	/*@
	  @ loop invariant t >= \at(t, Pre);
	  @ loop invariant quoting == 0 || quoting == 1;
	  @ loop invariant \valid_read(t);
	  @ loop assigns t, quoting;
	  @*/
	while(*t!='\0' && (quoting || utfrune(sep, *t)==nil)){
		if(*t != '\''){
			t++;
			continue;
		}
		/* *t is a quote */
		if(!quoting){
			quoting = 1;
			t++;
			continue;
		}
		/* quoting and we're on a quote */
		if(t[1] != '\''){
			/* end of quoted section; absorb closing quote */
			t++;
			quoting = 0;
			continue;
		}
		/* doubled quote; fold one quote into two */
		t += 2;
	}
	return t;
}

/*@
  @ requires maxargs >= 0;
  @ requires \valid(s + (0..));
  @ requires \valid(args + (0 .. maxargs-1));
  @ requires \valid_read(sep + (0..));
  @ requires \exists integer n1; n1 >= 0 && s[n1] == '\0';
  @ requires \exists integer n2; n2 >= 0 && sep[n2] == '\0';
  @ assigns s[0 .. \strlen(\at(s, Pre))], args[0 .. maxargs-1];
  @ ensures 0 <= \result <= maxargs;
  @ ensures \forall integer i; 0 <= i < \result ==>
  @           \valid_read(args[i] + (0..)) && args[i] >= \at(s, Pre);
  @*/
int
gettokens(char *s, char **args, int maxargs, char *sep)
{
	int nargs;

	/*@
	  @ loop invariant 0 <= nargs <= maxargs;
	  @ loop invariant s >= \at(s, Pre);
	  @ loop invariant \valid(s);
	  @ loop assigns nargs, s, \at(s, Pre)[0 .. \strlen(\at(s, Pre))], args[0 .. maxargs-1];
	  @ loop variant maxargs - nargs;
	  @*/
	for(nargs=0; nargs<maxargs; nargs++){
		/*@
		  @ loop invariant s >= \at(s, LoopEntry);
		  @ loop invariant \valid(s);
		  @ loop assigns s, \at(s, Pre)[0 .. \strlen(\at(s, Pre))];
		  @*/
		while(*s!='\0' && utfrune(sep, *s)!=nil)
			*s++ = '\0';
		if(*s == '\0')
			break;
		args[nargs] = s;
		s = etoken(s, sep);
	}

	return nargs;
}

/*@
  @ requires maxargs >= 0;
  @ requires \valid(s + (0..));
  @ requires \valid(args + (0 .. maxargs-1));
  @ requires \exists integer n; n >= 0 && s[n] == '\0';
  @ assigns s[0 .. \strlen(\at(s, Pre))], args[0 .. maxargs-1];
  @ ensures 0 <= \result <= maxargs;
  @ ensures \forall integer i; 0 <= i < \result ==>
  @           \valid_read(args[i] + (0..)) && args[i] >= \at(s, Pre);
  @*/
int
tokenize(char *s, char **args, int maxargs)
{
	int nargs;

	/*@
	  @ loop invariant 0 <= nargs <= maxargs;
	  @ loop invariant s >= \at(s, Pre);
	  @ loop invariant \valid(s);
	  @ loop assigns nargs, s, \at(s, Pre)[0 .. \strlen(\at(s, Pre))], args[0 .. maxargs-1];
	  @ loop variant maxargs - nargs;
	  @*/
	for(nargs=0; nargs<maxargs; nargs++){
		/*@
		  @ loop invariant s >= \at(s, LoopEntry);
		  @ loop invariant \valid(s);
		  @ loop assigns s;
		  @*/
		while(*s!='\0' && utfrune(qsep, *s)!=nil)
			s++;
		if(*s == '\0')
			break;
		args[nargs] = s;
		s = qtoken(s, qsep);
	}

	return nargs;
}
