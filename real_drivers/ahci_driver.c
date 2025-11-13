/*
 * Minimal AHCI driver stub for lux9-kernel
 * Provides the expected SDifc interface using existing definitions
 */

#include "../port/gcc_compat.h"

// Forward declarations
static SDev* ahci_pnp(void);
static int ahci_enable(SDev *s);
static int ahci_disable(SDev *s);
static int ahci_verify(SDunit *u);
static int ahci_online(SDunit *u);
static int ahci_io(SDreq *r);
static char* ahci_ctl(SDunit *u, char *p, char *e);
static int ahci_wctl(SDunit *u, void *cmd);
static long ahci_bio(SDunit *u, int lun, int write, void *a, long count, uvlong lba);
static char* ahci_topctl(SDev *s, char *p, char *e);
static int ahci_wtopctl(SDev *s, void *cmd);
static int ahci_ataio(SDreq *r);

// Export the driver interface
SDifc sdiahciifc = {
    "ahci",
    ahci_pnp,
    ahci_enable,
    ahci_disable,
    ahci_verify,
    ahci_online,
    ahci_io,
    ahci_ctl,
    ahci_wctl,
    ahci_bio,
    nil,        // probe
    nil,        // clear
    ahci_topctl,
    ahci_wtopctl,
    ahci_ataio,
};

// Stub implementations
static SDev* ahci_pnp(void) {
    // No AHCI devices found
    return nil;
}

static int ahci_enable(SDev *s) {
    return 1; // Always succeed
}

static int ahci_disable(SDev *s) {
    return 1; // Always succeed
}

static int ahci_verify(SDunit *u) {
    return 0; // Not present
}

static int ahci_online(SDunit *u) {
    return 0; // Not online
}

static int ahci_io(SDreq *r) {
    return -1; // Always fail
}

static char* ahci_ctl(SDunit *u, char *p, char *e) {
    return p; // No control data
}

static int ahci_wctl(SDunit *u, void *cmd) {
    return 0; // Success
}

static long ahci_bio(SDunit *u, int lun, int write, void *a, long count, uvlong lba) {
    return -1; // Always fail
}

static char* ahci_topctl(SDev *s, char *p, char *e) {
    return p; // No top-level control
}

static int ahci_wtopctl(SDev *s, void *cmd) {
    return 0; // Success
}

static int ahci_ataio(SDreq *r) {
    return -1; // Always fail
}
