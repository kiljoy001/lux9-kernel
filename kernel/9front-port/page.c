#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>
#include "pageown.h"

extern uintptr saved_limine_hhdm_offset;
extern uintptr* mmuwalk(uintptr*, uintptr, int, int);

static inline uintptr
hhdm_virt(uintptr pa)
{
	return pa + saved_limine_hhdm_offset;
}

Palloc palloc;

static void
dbgserial(int c)
{
}

static void
dbgserial_hex(uvlong v)
{
	int started, shift, nib;

	started = 0;
	for(shift = (int)(sizeof(uvlong)*8) - 4; shift >= 0; shift -= 4){
		nib = (v>>shift) & 0xF;
		if(!started && nib == 0 && shift != 0)
			continue;
		started = 1;
		dbgserial(nib < 10 ? '0'+nib : 'A'+nib-10);
	}
	if(!started)
		dbgserial('0');
}

ulong
nkpages(Confmem *cm)
{
	return ((cm->klimit - cm->kbase) + BY2PG-1) / BY2PG;
}

void
pageinit(void)
{
	int color, i, j;
	Page *p, **t;
	Confmem *cm;
	vlong m, v, u;
	ulong skipped_unmapped, skipped_poison;

	if(palloc.pages == nil){
		ulong np;

		np = 0;
		for(i=0; i<nelem(conf.mem); i++){
			cm = &conf.mem[i];
			np += cm->npage - nkpages(cm);
		}
		palloc.pages = xalloc(np*sizeof(Page));
		if(palloc.pages == nil)
			panic("pageinit");
	}

	color = 0;
	palloc.freecount = 0;
	palloc.head = nil;
	skipped_unmapped = 0;
	skipped_poison = 0;

	t = &palloc.head;
	p = palloc.pages;

	for(i=0; i<nelem(conf.mem); i++){
		cm = &conf.mem[i];
		if(cm->npage == 0 || cm->base == 0)
			continue;
		dbgserial('P');
		dbgserial('B');
		dbgserial('A' + (i%26));
		dbgserial('[');
		dbgserial_hex(cm->base);
		dbgserial(']');
		dbgserial('(');
		dbgserial_hex(cm->npage);
		dbgserial(')');
		for(j=nkpages(cm); j<cm->npage; j++){
			memset(p, 0, sizeof *p);
			p->pa = cm->base+j*BY2PG;
			if(cankaddr(p->pa) == 0){
				skipped_unmapped++;
				continue;
			}
			void *kva = KADDR(p->pa);
			if(kva == nil || kva == (void*)-BY2PG){
				skipped_poison++;
				continue;
			}
			p->color = color;
			color = (color+1)%NCOLOR;
			*t = p, t = &p->next;
			palloc.freecount++;
			p++;
		}
	}

	palloc.user = p - palloc.pages;
	u = palloc.user*BY2PG;
	v = u + conf.nswap*BY2PG;
	dbgserial('P');
	dbgserial('F');
	dbgserial_hex(palloc.freecount);
	dbgserial('U');
	dbgserial_hex(skipped_unmapped);
	dbgserial('I');
	dbgserial_hex(skipped_poison);
	dbgserial('\n');

	/* Paging numbers */
	swapalloc.highwater = (palloc.user*5)/100;
	swapalloc.headroom = swapalloc.highwater + (swapalloc.highwater/4);

	m = 0;
	for(i=0; i<nelem(conf.mem); i++)
		if(conf.mem[i].npage)
			m += conf.mem[i].npage*BY2PG;
	m += PGROUND(end - (char*)KTZERO);

	print("%lldM memory: ", (m+1024*1024-1)/(1024*1024));
	print("%lldM kernel data, ", (m-u+1024*1024-1)/(1024*1024));
	print("%lldM user, ", u/(1024*1024));
	print("%lldM swap\n", v/(1024*1024));
}

static void
pagechaindone(void)
{
	if(palloc.pwait[0].rendez.p != nil && wakeup(&palloc.pwait[0]) != nil)
		return;
	if(palloc.pwait[1].rendez.p != nil)
		wakeup(&palloc.pwait[1]);
}

void
freepages(Page *head, Page *tail, ulong np)
{
	if(head == nil)
		return;
	if(tail == nil){
		tail = head;
		for(np = 1;; np++){
			tail->ref = 0;
			if(tail->next == nil)
				break;
			tail = tail->next;
		}
	}
	lock(&palloc);
	tail->next = palloc.head;
	palloc.head = head;
	palloc.freecount += np;
	pagechaindone();
	unlock(&palloc);
}

ulong
pagereclaim(Image *i)
{
	Page **h, **e, **l, **x, *p;
	Page *fh, *ft;
	ulong mp, np;

	if(i == nil)
		return 0;

	lock(i);
	mp = i->pgref;
	if(mp == 0){
		unlock(i);
		return 0;
	}
	np = 0;
	fh = ft = nil;
	e = &i->pghash[i->pghsize];
	for(h = i->pghash; h < e; h++){
		l = h;
		x = nil;
		for(p = *l; p != nil; p = p->next){
			if(p->ref == 0)
				x = l;
			l = &p->next;
		}
		if(x == nil)
			continue;

		p = *x;
		*x = p->next;
		p->next = nil;
		p->image = nil;
		p->daddr = ~0;

		if(fh == nil)
			fh = p;
		else
			ft->next = p;
		ft = p;
		np++;

		if(--i->pgref == 0){
			putimage(i);
			goto Done;
		}
		decref(i);
	}
	unlock(i);
Done:
	freepages(fh, ft, np);
	return np;
}

static int
ispages(void*)
{
	return palloc.freecount > swapalloc.highwater || up->noswap && palloc.freecount > 0;
}

Page*
newpage(uintptr va, QLock *locked)
{
	Page *p, **l;
	int color;

	print("newpage: va=%p locked=%p\n", va, locked);
	print("newpage: palloc.freecount=%lud\n", palloc.freecount);
	lock(&palloc);
	print("newpage: acquired lock, checking ispages\n");
	while(!ispages(nil)){
		print("newpage: no pages available, waiting\n");
		unlock(&palloc);
		if(locked)
			qunlock(locked);

		if(!waserror()){
			Rendezq *q;

			q = &palloc.pwait[!up->noswap];
			eqlock(q);
			if(!waserror()){
				kickpager();
				sleep(q, ispages, nil);
				poperror();
			}
			qunlock(q);
			poperror();
		}

		/*
		 * If called from fault and we lost the lock from
		 * underneath don't waste time allocating and freeing
		 * a page. Fault will call newpage again when it has
		 * reacquired the locks
		 */
		if(locked)
			return nil;

		lock(&palloc);
	}

	print("newpage: passed ispages check\n");
	/* First try for our colour */
	print("newpage: calling getpgcolor\n");
	color = getpgcolor(va);
	print("newpage: color=%d palloc.head=%p\n", color, palloc.head);
	l = &palloc.head;
	print("newpage: searching for page with color=%d\n", color);
	for(p = *l; p != nil; p = p->next){
		if(p->color == color)
			break;
		l = &p->next;
	}
	print("newpage: search complete, p=%p\n", p);

	if(p == nil) {
		print("newpage: color not found, using first page\n");
		l = &palloc.head;
		p = *l;
		print("newpage: first page p=%p\n", p);
	}

	print("newpage: unlinking page from free list\n");
	*l = p->next;
	p->next = nil;
	palloc.freecount--;
	unlock(&palloc);
	print("newpage: unlocked palloc\n");

	print("newpage: setting page fields p=%p pa=%p\n", p, p->pa);
	p->ref = 1;
	p->va = va;
	p->modref = 0;
	print("newpage: calling inittxtflush\n");
	inittxtflush(p);
	print("newpage: inittxtflush complete\n");

	/* Automatically acquire ownership for the current process */
	/* With HHDM, use the HHDM virtual address instead of user VA */
	/* RE-ENABLED for debugging */
	if(up != nil && p->pa != 0) {
		extern uintptr saved_limine_hhdm_offset;
		uintptr hhdm_va = p->pa + saved_limine_hhdm_offset;
		print("newpage: calling pageown_acquire pa=%p hhdm_va=%p\n", p->pa, hhdm_va);
		pageown_acquire(up, p->pa, hhdm_va);
		print("newpage: pageown_acquire complete\n");
	}

	print("newpage: returning page %p\n", p);
	return p;
}

/*
 *  deadpage() decrements the page refcount
 *  and returns the page when it becomes freeable.
 */
Page*
deadpage(Page *p)
{
	if(p->image != nil){
		decref(p);
		return nil;
	}
	if(decref(p) != 0)
		return nil;
	return p;
}

void
putpage(Page *p)
{
	/* Release ownership before freeing */
	/* TEMPORARILY DISABLED - pageown lock is broken */
	if(0 && p != nil && up != nil && p->pa != 0) {
		pageown_release(up, p->pa);
	}

	p = deadpage(p);
	if(p != nil)
		freepages(p, p, 1);
}

void
copypage(Page *f, Page *t)
{
	KMap *ks, *kd;

	ks = kmap(f);
	kd = kmap(t);
	memmove((void*)VA(kd), (void*)VA(ks), BY2PG);
	kunmap(ks);
	kunmap(kd);
}

Page*
fillpage(Page *p, int c)
{
	KMap *k;

	if(p != nil){
		k = kmap(p);
		memset((void*)VA(k), c, BY2PG);
		kunmap(k);
	}
	return p;
}

void
cachepage(Page *p, Image *i)
{
	Page *x, **h;
	uintptr daddr;

	daddr = p->daddr;
	h = &PGHASH(i, daddr);
	lock(i);
	for(x = *h; x != nil; x = x->next)
		if(x->daddr == daddr)
			goto done;
	if(p->image != nil)
		goto done;
	p->image = i;
	p->next = *h;
	*h = p;
	incref(i);
	i->pgref++;
done:
	unlock(i);
}

void
uncachepage(Page *p)
{
	Page **l, *x;
	Image *i;

	i = p->image;
	if(i == nil)
		return;
	l = &PGHASH(i, p->daddr);
	lock(i);
	if(p->image != i)
		goto done;
	for(x = *l; x != nil; x = x->next) {
		if(x == p){
			*l = p->next;
			p->next = nil;
			p->image = nil;
			p->daddr = ~0;
			i->pgref--;
			putimage(i);
			return;
		}
		l = &x->next;
	}
done:
	unlock(i);
}

Page*
lookpage(Image *i, uintptr daddr)
{
	Page *p, **h, **l;

	l = h = &PGHASH(i, daddr);
	lock(i);
	for(p = *l; p != nil; p = p->next){
		if(p->daddr == daddr){
			*l = p->next;
			p->next = *h;
			*h = p;
			incref(p);
			unlock(i);
			return p;
		}
		l = &p->next;
	}
	unlock(i);

	return nil;
}

void
cachedel(Image *i, uintptr daddr)
{
	Page *p;

	if((p = lookpage(i, daddr)) != nil){
		uncachepage(p);
		putpage(p);
	}
}

void
zeroprivatepages(void)
{
	Page *p, *pe;

	/*
	 * in case of a panic, we might not have a process
	 * context to do the clearing of the private pages.
	 */
	if(up == nil){
		assert(panicking);
		return;
	}

	lock(&palloc);
	pe = palloc.pages + palloc.user;
	for(p = palloc.pages; p != pe; p++) {
		if(p->modref & PG_PRIV){
			incref(p);
			fillpage(p, 0);
			decref(p);
		}
	}
	unlock(&palloc);
}

/*
 * Map a user page at a specific virtual address.
 * This function creates the necessary page table entries
 * and links them into the current process's mmuhead.
 */
void
userpmap(uintptr va, uintptr pa, int perms)
{
	uintptr *pte;
	int x;

	x = splhi();
	pte = mmuwalk(m->pml4, va, 0, 1);  // level=0 (PTE), create=1
	if(pte == nil) {
		splx(x);
		panic("userpmap: out of memory for page tables");
	}
	*pte = pa | perms;
	print("userpmap: va=%#p pa=%#p perms=%#ux pte=%#llux\n",
		va, pa, perms, (uvlong)*pte);
	splx(x);
}
