#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "libc.h"
#include "fcall.h"

void
freefcall(Fcall *f)
{
	int i;

	switch(f->type) {
	case Tversion:
	case Rversion:
		free(f->version);
		break;
	case Tauth:
	case Tattach:
		free(f->uname);
		free(f->aname);
		break;
	case Twalk:
		for(i = 0; i < f->nwname; i++)
			free(f->wname[i]);
		break;
	case Topen:
	case Tcreate:
		free(f->name);
		break;
	case Rerror:
		free(f->ename);
		break;
	case Rread:
		free(f->data);
		break;
	}
}
