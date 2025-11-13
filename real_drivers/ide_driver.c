/*
 * Minimal IDE driver stub for lux9-kernel
 * Provides the expected SDifc interface using existing definitions
 */

#include "../port/gcc_compat.h"

// Forward declarations
static SDev* ide_pnp(void);
static int ide_enable(SDev *s);
static int ide_disable(SDev *s);
static int ide_verify(SDunit *u);
static int ide_online(SDunit *u);
static int ide_io(SDreq *r);
static char* ide_ctl(SDunit *u, char *p, char *e);
static int ide_wctl(SDunit *u, void *cmd);
static long ide_bio(SDunit *u, int lun, int write, void *a, long count, uvlong lba);
static char* ide_topctl(SDev *s, char *p, char *e);
static int ide_wtopctl(SDev *s, void *cmd);
static int ide_ataio(SDreq *r);

// Export the driver interface
SDifc sdideifc = {
    "ide",
    ide_pnp,
    ide_enable,
    ide_disable,
    ide_verify,
    ide_online,
    ide_io,
    ide_ctl,
    ide_wctl,
    ide_bio,
    nil,        // probe
    nil,        // clear
    ide_topctl,
    ide_wtopctl,
    ide_ataio,
};

// Stub implementations
static SDev* ide_pnp(void) {
    // No IDE devices found
    return nil;
}

static int ide_enable(SDev *s) {
    return 1; // Always succeed
}

static int ide_disable(SDev *s) {
    return 1; // Always succeed
}

static int ide_verify(SDunit *u) {
    return 0; // Not present
}

static int ide_online(SDunit *u) {
    return 0; // Not online
}

static int ide_io(SDreq *r) {
    return -1; // Always fail
}

static char* ide_ctl(SDunit *u, char *p, char *e) {
    return p; // No control data
}

static int ide_wctl(SDunit *u, void *cmd) {
    return 0; // Success
}

static long ide_bio(SDunit *u, int lun, int write, void *a, long count, uvlong lba) {
    return -1; // Always fail
}

static char* ide_topctl(SDev *s, char *p, char *e) {
    return p; // No top-level control
}

static int ide_wtopctl(SDev *s, void *cmd) {
    return 0; // Success
}

static int ide_ataio(SDreq *r) {
    return -1; // Always fail
}
