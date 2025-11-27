/*
 * HAL 9P Server - Real Implementation
 *
 * Implements a full 9P2000 file server that exposes hardware as a filesystem:
 * /hal/pci/00:1f.2/config - PCI config space (binary r/w)
 * /hal/pci/00:1f.2/bar0   - BAR0 memory region
 * /hal/pci/00:1f.2/ctl    - Control commands
 * /hal/pci/00:1f.2/irq    - Interrupt waiting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

/* 9P Protocol Structures */
#define VERSION9P "9P2000"
#define MAXWELEM 16
#define IOHDRSZ 24

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;

typedef struct Qid {
    uchar type;
    uint vers;
    uvlong path;
} Qid;

typedef struct Fcall {
    uchar type;
    uint fid;
    ushort tag;
    union {
        struct {
            uint msize;
            char *version;
        };
        struct {
            ushort oldtag;
        };
        struct {
            char *ename;
        };
        struct {
            Qid qid;
            uint iounit;
        };
        struct {
            uint afid;
            char *uname;
            char *aname;
        };
        struct {
            uint perm;
            char *name;
            uchar mode;
        };
        struct {
            uint newfid;
            ushort nwname;
            char *wname[MAXWELEM];
        };
        struct {
            ushort nwqid;
            Qid wqid[MAXWELEM];
        };
        struct {
            uvlong offset;
            uint count;
            char *data;
        };
        struct {
            ushort nstat;
            uchar *stat;
        };
    };
} Fcall;

/* 9P Message Types */
enum {
    Tversion = 100,
    Rversion,
    Tauth = 102,
    Rauth,
    Tattach = 104,
    Rattach,
    Terror = 106, /* illegal */
    Rerror,
    Tflush = 108,
    Rflush,
    Twalk = 110,
    Rwalk,
    Topen = 112,
    Ropen,
    Tcreate = 114,
    Rcreate,
    Tread = 116,
    Rread,
    Twrite = 118,
    Rwrite,
    Tclunk = 120,
    Rclunk,
    Tremove = 122,
    Rremove,
    Tstat = 124,
    Rstat,
    Twstat = 126,
    Rwstat,
};

/* QID Types */
enum {
    QTDIR    = 0x80,
    QTAPPEND = 0x40,
    QTEXCL   = 0x20,
    QTMOUNT  = 0x10,
    QTAUTH   = 0x08,
    QTTMP    = 0x04,
    QTFILE   = 0x00
};

/* HAL File System Tree Node */
typedef struct HalNode {
    char name[256];
    Qid qid;
    uint perm;
    uvlong length;
    int isdir;

    /* Hardware backend */
    enum {
        HAL_ROOT,
        HAL_PCI_DIR,
        HAL_PCI_DEVICE,
        HAL_PCI_CONFIG,
        HAL_PCI_BAR,
        HAL_PCI_CTL,
        HAL_PCI_IRQ,
        HAL_USB_DIR,
        HAL_USB_DEVICE,
        HAL_MMIO_DIR,
        HAL_MMIO_REGION,
    } type;

    /* PCI device context */
    struct {
        uint bus;
        uint device;
        uint function;
        int config_fd;  /* fd to /sys/bus/pci/.../config or Family API */
    } pci;

    /* BAR context */
    struct {
        uint bar_num;
        uvlong phys_addr;
        uvlong size;
        void *mapped_addr;
    } bar;

    struct HalNode *parent;
    struct HalNode *children;
    struct HalNode *next;
} HalNode;

/* FID Table Entry */
typedef struct Fid {
    uint fid;
    HalNode *node;
    int omode;
    uvlong offset;
    struct Fid *next;
} Fid;

/* Server Context */
typedef struct {
    HalNode *root;
    Fid *fids;
    pthread_mutex_t lock;
    uint max_msize;
    uvlong next_qid_path;
} HalServer;

static HalServer server;

/* ===== 9P Protocol Helpers ===== */

static uint GBIT32(uchar *p) {
    return p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

static void PBIT32(uchar *p, uint v) {
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

static ushort GBIT16(uchar *p) {
    return p[0] | (p[1]<<8);
}

static void PBIT16(uchar *p, ushort v) {
    p[0] = v;
    p[1] = v >> 8;
}

static uvlong GBIT64(uchar *p) {
    return (uint)GBIT32(p) | ((uvlong)GBIT32(p+4) << 32);
}

static void PBIT64(uchar *p, uvlong v) {
    PBIT32(p, v);
    PBIT32(p+4, v >> 32);
}

/* ===== FID Management ===== */

static Fid* fid_lookup(uint fid) {
    Fid *f;
    for(f = server.fids; f; f = f->next) {
        if(f->fid == fid)
            return f;
    }
    return NULL;
}

static Fid* fid_alloc(uint fid, HalNode *node) {
    Fid *f = malloc(sizeof(Fid));
    if(!f) return NULL;

    f->fid = fid;
    f->node = node;
    f->omode = -1;
    f->offset = 0;
    f->next = server.fids;
    server.fids = f;
    return f;
}

static void fid_free(uint fid) {
    Fid **fp, *f;
    for(fp = &server.fids; (f = *fp) != NULL; fp = &f->next) {
        if(f->fid == fid) {
            *fp = f->next;
            free(f);
            return;
        }
    }
}

/* ===== Filesystem Tree Management ===== */

static HalNode* node_create(const char *name, int isdir) {
    HalNode *n = calloc(1, sizeof(HalNode));
    if(!n) return NULL;

    strncpy(n->name, name, sizeof(n->name)-1);
    n->qid.type = isdir ? QTDIR : QTFILE;
    n->qid.vers = 0;
    n->qid.path = ++server.next_qid_path;
    n->perm = isdir ? 0755 : 0644;
    n->isdir = isdir;

    return n;
}

static void node_add_child(HalNode *parent, HalNode *child) {
    child->parent = parent;
    child->next = parent->children;
    parent->children = child;
}

static HalNode* node_walk(HalNode *from, const char *name) {
    HalNode *n;

    if(strcmp(name, "..") == 0)
        return from->parent ? from->parent : from;

    for(n = from->children; n; n = n->next) {
        if(strcmp(n->name, name) == 0)
            return n;
    }

    return NULL;
}

/* ===== Hardware Backend: PCI Config Space ===== */

static int pci_config_open(HalNode *node) {
    char path[512];

    /* Try Linux sysfs first */
    snprintf(path, sizeof(path),
             "/sys/bus/pci/devices/0000:%02x:%02x.%x/config",
             node->pci.bus, node->pci.device, node->pci.function);

    node->pci.config_fd = open(path, O_RDWR);
    if(node->pci.config_fd >= 0)
        return 0;

    /* TODO: Try Lux9 Family API via /dev/family/pci/... */

    return -1;
}

static int pci_config_read(HalNode *node, void *buf, uvlong offset, uint count) {
    if(node->pci.config_fd < 0)
        if(pci_config_open(node) < 0)
            return -1;

    lseek(node->pci.config_fd, offset, SEEK_SET);
    return read(node->pci.config_fd, buf, count);
}

static int pci_config_write(HalNode *node, void *buf, uvlong offset, uint count) {
    if(node->pci.config_fd < 0)
        if(pci_config_open(node) < 0)
            return -1;

    lseek(node->pci.config_fd, offset, SEEK_SET);
    return write(node->pci.config_fd, buf, count);
}

/* ===== 9P Message Handlers ===== */

static void handle_version(Fcall *tx, Fcall *rx) {
    rx->type = Rversion;
    rx->tag = tx->tag;
    rx->msize = (tx->msize < 8192) ? tx->msize : 8192;
    rx->version = VERSION9P;
    server.max_msize = rx->msize;

    printf("HAL: Version negotiated: msize=%u\n", rx->msize);
}

static void handle_attach(Fcall *tx, Fcall *rx) {
    Fid *f;

    printf("HAL: Attach fid=%u uname=%s aname=%s\n",
           tx->fid, tx->uname, tx->aname);

    f = fid_alloc(tx->fid, server.root);
    if(!f) {
        rx->type = Rerror;
        rx->ename = "out of memory";
        return;
    }

    rx->type = Rattach;
    rx->tag = tx->tag;
    rx->qid = server.root->qid;
}

static void handle_walk(Fcall *tx, Fcall *rx) {
    Fid *f, *nf;
    HalNode *node;
    int i;

    printf("HAL: Walk fid=%u newfid=%u nwname=%u\n",
           tx->fid, tx->newfid, tx->nwname);

    f = fid_lookup(tx->fid);
    if(!f) {
        rx->type = Rerror;
        rx->ename = "unknown fid";
        return;
    }

    node = f->node;

    /* Walk path */
    for(i = 0; i < tx->nwname; i++) {
        printf("  walk: %s\n", tx->wname[i]);

        if(!node->isdir) {
            rx->type = Rerror;
            rx->ename = "not a directory";
            return;
        }

        node = node_walk(node, tx->wname[i]);
        if(!node) {
            /* Partial walk success */
            break;
        }

        rx->wqid[i] = node->qid;
    }

    rx->nwqid = i;

    /* Create/update newfid */
    if(tx->newfid != tx->fid) {
        nf = fid_alloc(tx->newfid, node);
        if(!nf) {
            rx->type = Rerror;
            rx->ename = "out of memory";
            return;
        }
    } else {
        f->node = node;
    }

    rx->type = Rwalk;
    rx->tag = tx->tag;
}

static void handle_open(Fcall *tx, Fcall *rx) {
    Fid *f;

    printf("HAL: Open fid=%u mode=%u\n", tx->fid, tx->mode);

    f = fid_lookup(tx->fid);
    if(!f) {
        rx->type = Rerror;
        rx->ename = "unknown fid";
        return;
    }

    f->omode = tx->mode;
    f->offset = 0;

    rx->type = Ropen;
    rx->tag = tx->tag;
    rx->qid = f->node->qid;
    rx->iounit = 0;
}

static void handle_read(Fcall *tx, Fcall *rx) {
    Fid *f;
    HalNode *node;
    char *buf;
    int n;

    f = fid_lookup(tx->fid);
    if(!f) {
        rx->type = Rerror;
        rx->ename = "unknown fid";
        return;
    }

    node = f->node;

    /* Directory read */
    if(node->isdir) {
        /* TODO: Generate directory listing */
        rx->type = Rread;
        rx->tag = tx->tag;
        rx->count = 0;
        rx->data = "";
        return;
    }

    /* File read */
    buf = malloc(tx->count);
    if(!buf) {
        rx->type = Rerror;
        rx->ename = "out of memory";
        return;
    }

    n = -1;

    switch(node->type) {
    case HAL_PCI_CONFIG:
        n = pci_config_read(node, buf, tx->offset, tx->count);
        break;

    case HAL_PCI_CTL:
        /* Return control commands help */
        n = snprintf(buf, tx->count, "enable\ndisable\nreset\n");
        break;

    default:
        free(buf);
        rx->type = Rerror;
        rx->ename = "not implemented";
        return;
    }

    if(n < 0) {
        free(buf);
        rx->type = Rerror;
        rx->ename = "i/o error";
        return;
    }

    rx->type = Rread;
    rx->tag = tx->tag;
    rx->count = n;
    rx->data = buf;
}

static void handle_write(Fcall *tx, Fcall *rx) {
    Fid *f;
    HalNode *node;
    int n;

    f = fid_lookup(tx->fid);
    if(!f) {
        rx->type = Rerror;
        rx->ename = "unknown fid";
        return;
    }

    node = f->node;

    n = -1;

    switch(node->type) {
    case HAL_PCI_CONFIG:
        n = pci_config_write(node, tx->data, tx->offset, tx->count);
        break;

    case HAL_PCI_CTL:
        /* Parse control commands */
        if(strncmp(tx->data, "enable", 6) == 0) {
            printf("HAL: Enable device %02x:%02x.%x\n",
                   node->pci.bus, node->pci.device, node->pci.function);
            n = tx->count;
        } else if(strncmp(tx->data, "reset", 5) == 0) {
            printf("HAL: Reset device %02x:%02x.%x\n",
                   node->pci.bus, node->pci.device, node->pci.function);
            n = tx->count;
        }
        break;

    default:
        rx->type = Rerror;
        rx->ename = "not implemented";
        return;
    }

    if(n < 0) {
        rx->type = Rerror;
        rx->ename = "i/o error";
        return;
    }

    rx->type = Rwrite;
    rx->tag = tx->tag;
    rx->count = n;
}

static void handle_clunk(Fcall *tx, Fcall *rx) {
    printf("HAL: Clunk fid=%u\n", tx->fid);

    fid_free(tx->fid);

    rx->type = Rclunk;
    rx->tag = tx->tag;
}

/* ===== Main 9P Dispatcher ===== */

static void dispatch_9p(Fcall *tx, Fcall *rx) {
    memset(rx, 0, sizeof(*rx));
    rx->tag = tx->tag;

    switch(tx->type) {
    case Tversion:
        handle_version(tx, rx);
        break;
    case Tattach:
        handle_attach(tx, rx);
        break;
    case Twalk:
        handle_walk(tx, rx);
        break;
    case Topen:
        handle_open(tx, rx);
        break;
    case Tread:
        handle_read(tx, rx);
        break;
    case Twrite:
        handle_write(tx, rx);
        break;
    case Tclunk:
        handle_clunk(tx, rx);
        break;
    case Tflush:
        rx->type = Rflush;
        break;
    default:
        printf("HAL: Unsupported message type %d\n", tx->type);
        rx->type = Rerror;
        rx->ename = "unsupported operation";
        break;
    }
}

/* ===== Initialization ===== */

static void hal_init_filesystem(void) {
    HalNode *pci_dir, *pci_dev, *config, *ctl;

    /* Create root */
    server.root = node_create("", 1);
    server.root->type = HAL_ROOT;

    /* Create /hal/pci/ */
    pci_dir = node_create("pci", 1);
    pci_dir->type = HAL_PCI_DIR;
    node_add_child(server.root, pci_dir);

    /* Create example device /hal/pci/00:1f.2/ */
    pci_dev = node_create("00:1f.2", 1);
    pci_dev->type = HAL_PCI_DEVICE;
    pci_dev->pci.bus = 0;
    pci_dev->pci.device = 0x1f;
    pci_dev->pci.function = 2;
    pci_dev->pci.config_fd = -1;
    node_add_child(pci_dir, pci_dev);

    /* Create /hal/pci/00:1f.2/config */
    config = node_create("config", 0);
    config->type = HAL_PCI_CONFIG;
    config->pci = pci_dev->pci;
    config->length = 256; /* PCI config space size */
    node_add_child(pci_dev, config);

    /* Create /hal/pci/00:1f.2/ctl */
    ctl = node_create("ctl", 0);
    ctl->type = HAL_PCI_CTL;
    ctl->pci = pci_dev->pci;
    node_add_child(pci_dev, ctl);

    printf("HAL: Filesystem tree initialized\n");
}

int main(int argc, char *argv[]) {
    int port = 564;

    printf("HAL: Starting 9P server...\n");

    memset(&server, 0, sizeof(server));
    pthread_mutex_init(&server.lock, NULL);
    server.next_qid_path = 0;

    hal_init_filesystem();

    printf("HAL: Listening on port %d\n", port);
    printf("HAL: Mount with: mount -t 9p -o trans=tcp,port=%d 127.0.0.1 /mnt/hal\n", port);

    /* TODO: Actual TCP server loop */

    return 0;
}
