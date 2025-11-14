#include <u.h>
#include <libc.h>

static int
isdelim(int c, char *sep)
{
	return strchr(sep, c) != nil;
}

int
getfields(char *str, char **fields, int n, int skip, char *sep)
{
	char *s, *t;
	int nf;

	if(str == nil || fields == nil || sep == nil || n <= 0)
		return 0;

	s = str;
	nf = 0;
	while(nf < n){
		if(skip){
			while(*s != '\0' && isdelim(*s, sep))
				s++;
			if(*s == '\0')
				break;
		}
		fields[nf++] = s;
		t = s;
		while(*t != '\0' && !isdelim(*t, sep))
			t++;
		if(*t == '\0')
			break;
		*t++ = '\0';
		s = t;
	}

	return nf;
}
