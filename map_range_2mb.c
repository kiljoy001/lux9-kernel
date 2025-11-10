static void
map_range_2mb(u64int *pml4, u64int virt_start, u64int phys_start, u64int size, u64int perms)
{
	extern uintptr saved_limine_hhdm_offset;
	u64int virt, phys, virt_end;
	u64int pml4_idx, pdp_idx, pd_idx;
	u64int *pdp, *pd;
	u64int pdp_phys, pd_phys;

	virt_start &= ~0x1FFFFFULL;  /* Align to 2MB */
	phys_start &= ~0x1FFFFFULL;
	size = (size + 0x1FFFFF) & ~0x1FFFFFULL;

	virt_end = virt_start + size;

	/* Handle address space wrap: if virt_end wrapped to a small value, we're mapping to end of address space */
	int wraps = (virt_end < virt_start);

	for(virt = virt_start, phys = phys_start; wraps ? (virt >= virt_start) : (virt < virt_end); virt += 2*MiB, phys += 2*MiB) {
		/* Calculate indices */
		pml4_idx = (virt >> 39) & 0x1FF;
		pdp_idx = (virt >> 30) & 0x1FF;
		pd_idx = (virt >> 21) & 0x1FF;

		/* Get or create PDP */
		if((pml4[pml4_idx] & PTEVALID) == 0) {
			pdp = alloc_pt();
			pdp_phys = virt2phys(pdp);
			pml4[pml4_idx] = pdp_phys | PTEVALID | PTEWRITE | PTEACCESSED;
		} else {
			pdp_phys = pml4[pml4_idx] & ~0xFFF;
			pdp = (u64int*)hhdm_virt(pdp_phys);
		}

		/* Get or create PD */
		if((pdp[pdp_idx] & PTEVALID) == 0) {
			pd = alloc_pt();
			pd_phys = virt2phys(pd);
			pdp[pdp_idx] = pd_phys | PTEVALID | PTEWRITE | PTEACCESSED;
		} else {
			pd_phys = pdp[pdp_idx] & ~0xFFF;
			pd = (u64int*)hhdm_virt(pd_phys);
		}

		/* Map 2MB page directly in PD */
		pd[pd_idx] = phys | perms;  /* perms should include PTESIZE */
	}
}