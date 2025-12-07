#include "kernel_mock.h"

// From kernel/9front-port/page.c (simplified)
Page*
newpage(uintptr va, Image *image)
{
	Page *p = mallocz(sizeof(Page), 1);
	if(p == nil)
		panic("newpage: out of mock memory");
	
	p->pa = rampage(); // Get a mock physical address
	p->va = (uintptr*)hhdm_virt(p->pa); // Get mock HHDM virtual address

	// For userspace test, we don't simulate image association fully
	// (void)image; 

	return p;
}

void
free_page_mock(Page *p)
{
    if (p) {
        // In kernel, this would involve freeing the physical page.
        // In mock, we just free the struct.
        free(p);
    }
}