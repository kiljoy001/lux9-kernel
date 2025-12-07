#include "kernel_mock.h"
// Forward declarations for functions from mmu_mock.c
extern void pmap(uintptr pa, uintptr va, vlong size);
extern void punmap(uintptr va, vlong size); // Not used here yet, but good practice


// From kernel/9front-port/segment.c (simplified)
Segment*
newseg(int type, uintptr base, ulong size)
{
	Segment *s;

	s = mallocz(sizeof(Segment), 1);
	if(s == nil)
		return nil;
	s->base = base;
	s->top = base + size * BY2PG;
	s->size = size;
	s->ref = 1;
	s->type = type;
	s->map = mallocz(sizeof(Pte*) * 16, 1); // Mock initial map size
	s->mapsize = 16; // Mock initial mapsize
    
    // Simulate initial mapping of base segment page
    // For now, we only care about its existence and attributes
    
	return s;
}

// From kernel/9front-port/segment.c (simplified)
void
putseg(Segment *s)
{
	if(s == nil || s->ref == 0)
		return;
	
	if(decref(&s->ref) == 0){
        // In kernel, this would involve unmapping pages and freeing resources
        // For mock, just free the segment structure itself and its map
        if(s->map) free(s->map);
		free(s);
	}
}

// From kernel/9front-port/segment.c (simplified)
void
segpage(Segment *s, Page *p)
{
	if(s == nil || p == nil)
		return;

    // Simulate mapping the page into the segment's virtual address space
    // This calls pmap internally in kernel. We'll do the same.
    // Assuming the base of the segment is the VA where the first page is mapped.
    // Size is 1 page.
    pmap(p->pa | (PTEWRITE|PTEUSER), s->base, BY2PG);
}