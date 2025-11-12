#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>

#define MAXCONF 64

static char *confname[MAXCONF];
static char *confval[MAXCONF];
static int nconf;

static int
cistringeq(char *a, char *b)
{
	return cistrncmp(a, b, strlen(a)+1) == 0;
}

static void
parsebootargs(char *cp)
{
	char *line[MAXCONF];
	char *p, *q;
	int i, j, n;

	if(cp == nil)
		return;

	cp[BOOTARGSLEN-1] = 0;

	p = cp;
	for(q = cp; *q; q++){
		if(*q == '\r')
			continue;
		if(*q == '\t')
			*q = ' ';
		*p++ = *q;
	}
	*p = 0;

	n = 0;
	p = cp;
	while(n < MAXCONF && *p != '\0'){
		while(*p == '\n')
			p++;
		if(*p == '\0')
			break;
		line[n++] = p;
		while(*p != '\0' && *p != '\n')
			p++;
		if(*p == '\n')
			*p++ = '\0';
	}

	for(i = 0; i < n; i++){
		char *name;

		name = line[i];
		while(*name == ' ' || *name == '\t')
			name++;
		if(*name == '\0' || *name == '#')
			continue;
		p = strchr(name, '=');
		if(p == nil)
			continue;
		*p++ = '\0';
		while(*p == ' ' || *p == '\t')
			p++;
		for(j = 0; j < nconf; j++){
			if(cistringeq(confname[j], name))
				break;
		}
		confname[j] = name;
		confval[j] = p;
		if(j == nconf && nconf < MAXCONF)
			nconf++;
	}
}

void
bootconfinit(void)
{
	char *p;

	nconf = 0;
	parsebootargs(BOOTARGS);

	/* Check for kprintqsize boot parameter */
	if((p = getconf("kprintqsize")) != nil)
		setkprintqsize(p);
}

char*
getconf(char *name)
{
	int i;

	for(i = 0; i < nconf; i++)
		if(cistringeq(confname[i], name))
			return confval[i];
	return nil;
}

void
setconfenv(void)
{
	int i;

	for(i = 0; i < nconf; i++){
		if(confname[i][0] != '*')
			ksetenv(confname[i], confval[i], 0);
		ksetenv(confname[i], confval[i], 1);
	}
}

void
writeconf(void)
{
	char *p, *q;
	int n;

	p = getconfenv();
	if(waserror()) {
		free(p);
		nexterror();
	}

	for(q = p; *q; q++){
		q += strlen(q);
		*q = '=';
		q += strlen(q);
		*q = '\n';
	}
	n = q - p + 1;
	if(n >= BOOTARGSLEN)
		error("kernel configuration too large");
	memmove(BOOTARGS, p, n);
	memset(BOOTLINE, 0, BOOTLINELEN);
	poperror();
	free(p);
}
