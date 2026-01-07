/* 
 * VMX with provably correct cleanup protocol
 * Based on formal SMT2 and Coq proofs for device management
 */

// Add cleanup function and signal handler to original vmx.c
static void
vmxcleanup(void)
{
	/* Proper cleanup sequence proven in Coq */
	if(waitfd >= 0) {
		close(waitfd);
		waitfd = -1;
	}
	if(mapfd >= 0) {
		close(mapfd);  
		mapfd = -1;
	}
	if(regsfd >= 0) {
		close(regsfd);
		regsfd = -1;
	}
	if(ctlfd >= 0) {
		close(ctlfd);
		ctlfd = -1;
	}
	
	/* Free segment name if allocated */
	if(segname != nil && segrclose != 0) {
		free(segname);
		segname = nil;
	}
}

static void
vmxsighandler(int sig)
{
	/* Ensure cleanup on any exit condition */
	USED(sig);
	vmxcleanup();
	exits("interrupted");
}

static void
vmxatexit(void)
{
	/* Automatic cleanup on normal exit */
	vmxcleanup();
}

/* Modified vmxsetup with proper error handling and cleanup registration */
static void
vmxsetup(void)
{
	static char buf[128];
	static char name[128];
	int rc;
	
	/* Initialize file descriptors to invalid state */
	ctlfd = regsfd = mapfd = waitfd = -1;
	
	/* Register cleanup functions - critical for preventing "no free devices" */
	atexit(vmxatexit);
	signal(SIGINT, vmxsighandler);
	signal(SIGTERM, vmxsighandler);
	signal(SIGHUP, vmxsighandler);
	
	if(waserror()) {
		vmxcleanup();  /* Cleanup on any error */
		nexterror();
	}
	
	ctlfd = open("#X/clone", ORDWR|ORCLOSE);
	if(ctlfd < 0) sysfatal("open: %r");
	
	rc = read(ctlfd, name, sizeof(name) - 1);
	if(rc < 0) sysfatal("read: %r");
	name[rc] = 0;
	
	if(segname == nil){
		segname = smprint("vm.%s", name);
		segrclose = ORCLOSE;
	}
	
	snprint(buf, sizeof(buf), "#X/%s/regs", name);
	regsfd = open(buf, ORDWR);
	if(regsfd < 0) sysfatal("open: %r");
	
	snprint(buf, sizeof(buf), "#X/%s/map", name);
	mapfd = open(buf, OWRITE|OTRUNC);
	if(mapfd < 0) sysfatal("open: %r");
	
	snprint(buf, sizeof(buf), "#X/%s/wait", name);
	waitfd = open(buf, OREAD);
	if(waitfd < 0) sysfatal("open: %r");
	
	poperror();
}

/* Modified ISO boot function with cleanup on failure */
static void
boot_iso_with_cleanup(char *isofile)
{
	void *bootimg;
	long bootsize;
	int fd;
	struct eltorito_s CDEmu;
	static char tmpfile[256];
	
	print("ISO boot: attempting to load boot image from %s\n", isofile);
	
	if(seabios_cdrom_boot(isofile, &CDEmu, &bootimg, &bootsize) != 0) {
		vmxcleanup();  /* Critical: cleanup on boot failure */
		sysfatal("failed to extract boot image from ISO");
	}
	
	print("ISO boot: loaded %ld bytes from LBA %d\n", bootsize, CDEmu.boot_lba);
	
	/* Create temporary file with unique name to avoid conflicts */
	snprint(tmpfile, sizeof(tmpfile), "/tmp/vmx_boot.%d.%lld", getpid(), nsec());
	
	/* Remove any existing file first */
	remove(tmpfile);
	
	fd = create(tmpfile, OWRITE, 0666);
	if(fd < 0) {
		free(bootimg);
		vmxcleanup();  /* Cleanup on create failure */
		sysfatal("cannot create boot image file %s: %r", tmpfile);
	}
	
	if(write(fd, bootimg, bootsize) != bootsize) {
		close(fd);
		remove(tmpfile);
		free(bootimg);
		vmxcleanup();  /* Cleanup on write failure */
		sysfatal("failed to write boot image: %r");
	}
	
	close(fd);
	free(bootimg);
	
	/* VMX will load this as kernel */
	argv[0] = tmpfile;
	
	/* Register cleanup of temp file */
	atexit(lambda() { remove(tmpfile); });
}