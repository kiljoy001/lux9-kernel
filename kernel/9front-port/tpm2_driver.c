/*
 * TPM 2.0 Driver for Lux9
 *
 * Implements TPM 2.0 command interface via TIS (TPM Interface Specification)
 * Supports: Random, NVRAM, PCR, HMAC operations
 *
 * Hardware Interface: Memory-mapped I/O at 0xFED40000 (standard TPM base)
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "io.h"
#include "tpm.h"

/* TPM 2.0 Command Codes (avoid tpm.h conflicts) */
#define TPM2_CC_GetRandom_LOCAL     0x0000017B
#define TPM2_CC_PCR_Extend_LOCAL    0x00000182
#define TPM2_CC_PCR_Read_LOCAL      0x0000017E
#define TPM_SUCCESS_LOCAL           0x00000000

/* TPM 2.0 TIS Register Offsets (locality 0) */
#define TPM_ACCESS_0        0x0000  /* Access register */
#define TPM_STS_0           0x0018  /* Status register */
#define TPM_DATA_FIFO_0     0x0024  /* Data FIFO */
#define TPM_DID_VID_0       0x0F00  /* Device/Vendor ID */

/* TPM Access Register Bits */
#define TPM_ACCESS_VALID            0x80
#define TPM_ACCESS_ACTIVE_LOCALITY  0x20
#define TPM_ACCESS_REQUEST_USE      0x02
#define TPM_ACCESS_REQUEST_PENDING  0x04

/* TPM Status Register Bits */
#define TPM_STS_VALID       0x80
#define TPM_STS_COMMAND_READY 0x40
#define TPM_STS_TPM_GO      0x20
#define TPM_STS_DATA_AVAIL  0x10
#define TPM_STS_EXPECT      0x08
#define TPM_STS_BURST_MASK  0xFFFF00

/* TPM 2.0 Command Header */
typedef struct {
    u16int tag;
    u32int size;
    u32int code;
} __attribute__((packed)) TPM2_Command_Header;

/* TPM 2.0 Response Header */
typedef struct {
    u16int tag;
    u32int size;
    u32int code;
} __attribute__((packed)) TPM2_Response_Header;

/* Global TPM state */
static struct {
    int initialized;
    uintptr base;
    int version;  /* 2 for TPM 2.0 */
} tpm_state;

/* Helper: Read TPM register */
static u8int
tpm_read8(uintptr offset)
{
    volatile u8int *reg = (volatile u8int*)(tpm_state.base + offset);
    return *reg;
}

/* Helper: Write TPM register */
static void
tpm_write8(uintptr offset, u8int val)
{
    volatile u8int *reg = (volatile u8int*)(tpm_state.base + offset);
    *reg = val;
}

/* Helper: Read 32-bit TPM register */
static u32int
tpm_read32(uintptr offset)
{
    volatile u32int *reg = (volatile u32int*)(tpm_state.base + offset);
    return *reg;
}

/* Helper: Write 32-bit TPM register */
static void
tpm_write32(uintptr offset, u32int val)
{
    volatile u32int *reg = (volatile u32int*)(tpm_state.base + offset);
    *reg = val;
}

/* Wait for TPM status bit */
static int
tpm_wait_status(uintptr reg_offset, u8int mask, u8int expected, int timeout_ms)
{
    int i;
    u8int status;

    for(i = 0; i < timeout_ms; i++){
        status = tpm_read8(reg_offset);
        if((status & mask) == expected)
            return 0;

        /* Sleep ~1ms */
        microdelay(1000);
    }

    return -1;  /* Timeout */
}

/* Request TPM locality */
static int
tpm_request_locality(void)
{
    u8int access;

    /* Check if already active */
    access = tpm_read8(TPM_ACCESS_0);
    print("TPM: tpm_request_locality: Initial TPM_ACCESS_0 = 0x%x\n", access);
    if(access & TPM_ACCESS_ACTIVE_LOCALITY)
        return 0;

    /* Request use */
    tpm_write8(TPM_ACCESS_0, TPM_ACCESS_REQUEST_USE);

    /* Wait for locality */
    if(tpm_wait_status(TPM_ACCESS_0, TPM_ACCESS_ACTIVE_LOCALITY, TPM_ACCESS_ACTIVE_LOCALITY, 1000) < 0){
        access = tpm_read8(TPM_ACCESS_0);
        print("TPM: Failed to acquire locality (final TPM_ACCESS_0 = 0x%x)\n", access);
        return -1;
    }

    return 0;
}

/* Release TPM locality */
static void
tpm_release_locality(void)
{
    u8int access = TPM_ACCESS_ACTIVE_LOCALITY;
    tpm_write8(TPM_ACCESS_0, access);
}

/* Get burst count (how many bytes can be written to FIFO) */
static int
tpm_get_burst_count(void)
{
    u32int status;
    int burst;
    int i;

    for(i = 0; i < 1000; i++){
        status = tpm_read32(TPM_STS_0);
        burst = (status & TPM_STS_BURST_MASK) >> 8;
        if(burst > 0)
            return burst;
        microdelay(100);
    }

    return 0;
}

/*
 * Transmit command to TPM and receive response
 *
 * This is the core command transmission layer.
 * Made non-static for use by tpm2_sapi_minimal.c
 */
int
tpm_transmit(TPMContext* ctx, u8int *cmd, usize cmd_len, u8int *resp, usize *resp_len)
{
    USED(ctx);
    int i, burst, count;
    u8int status;
    TPM2_Response_Header *rhdr;

    if(!tpm_state.initialized){
        print("TPM: Not initialized\n");
        return -1;
    }

    if(cmd_len < 10 || cmd_len > 4096){
        print("TPM: Invalid command length %lud\n", cmd_len);
        return -1;
    }

    /* DEBUG: Dump command header */
    print("TPM CMD: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
          cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], 
          cmd[5], cmd[6], cmd[7], cmd[8], cmd[9]);

    /* Request locality */
    if(tpm_request_locality() < 0)
        return -1;

    /* Wait for command ready */
    status = tpm_read8(TPM_STS_0);
    if(!(status & TPM_STS_COMMAND_READY)){
        /* Send command ready */
        tpm_write8(TPM_STS_0, TPM_STS_COMMAND_READY);
        if(tpm_wait_status(TPM_STS_0, TPM_STS_COMMAND_READY, TPM_STS_COMMAND_READY, 1000) < 0){
            print("TPM: Timeout waiting for command ready\n");
            tpm_release_locality();
            return -1;
        }
    }

    /* Write command to FIFO */
    i = 0;
    while(i < cmd_len){
        burst = tpm_get_burst_count();
        if(burst == 0){
            print("TPM: Burst count timeout\n");
            tpm_release_locality();
            return -1;
        }

        count = (cmd_len - i < burst) ? cmd_len - i : burst;
        while(count-- > 0)
            tpm_write8(TPM_DATA_FIFO_0, cmd[i++]);
    }

    /* Verify EXPECT bit is clear */
    status = tpm_read8(TPM_STS_0);
    if(status & TPM_STS_EXPECT){
        print("TPM: EXPECT bit still set after writing command\n");
        tpm_release_locality();
        return -1;
    }

    /* Execute command */
    tpm_write8(TPM_STS_0, TPM_STS_TPM_GO);

    /* Wait for data available (up to 30 seconds for slow commands) */
    if(tpm_wait_status(TPM_STS_0, TPM_STS_DATA_AVAIL | TPM_STS_VALID,
                       TPM_STS_DATA_AVAIL | TPM_STS_VALID, 30000) < 0){
        print("TPM: Timeout waiting for response\n");
        tpm_release_locality();
        return -1;
    }

    /* Read response header to get total length */
    for(i = 0; i < 10; i++)
        resp[i] = tpm_read8(TPM_DATA_FIFO_0);

    rhdr = (TPM2_Response_Header*)resp;
    u32int resp_size = (rhdr->size >> 24) | ((rhdr->size >> 8) & 0xFF00) |
                       ((rhdr->size << 8) & 0xFF0000) | (rhdr->size << 24);

    if(resp_size < 10 || resp_size > *resp_len){
        print("TPM: Invalid response size %ud\n", resp_size);
        tpm_release_locality();
        return -1;
    }

    /* Read remaining bytes */
    for(i = 10; i < resp_size; i++){
        burst = tpm_get_burst_count();
        if(burst == 0){
            print("TPM: Burst timeout during response read\n");
            break;
        }
        resp[i] = tpm_read8(TPM_DATA_FIFO_0);
    }

    *resp_len = resp_size;

    /* Release locality */
    tpm_release_locality();

    /* NOTE: We don't check the response code here - let the caller handle it.
     * Some commands may expect non-zero response codes (e.g., TPM2_Startup
     * returns TPM_RC_INITIALIZE if already started, which is not an error). */

    return 0;
}

/*
 * Detect and initialize TPM hardware
 */
void
tpminit(void)
{
    u32int did_vid;
    uintptr phys_base = 0xFED40000;  /* Standard TPM base address */
    uintptr virt_base;  /* Use uintptr instead of void* to avoid pointer truncation */
    ulong tpm_size;

    print("TPM: Initializing TPM 2.0 driver...\n");

    /* TPM TIS specification defines 5KB of MMIO space per locality (locality 0-4)
     * We need at least 4KB for locality 0, round up to page boundary (4KB) */
    tpm_size = PGROUND(16*1024);  /* Map 16KB to cover all localities, page-aligned */

    /* Map TPM MMIO region using vmap() */
    virt_base = (uintptr)vmap(phys_base, tpm_size);
    if(virt_base == 0){
        print("TPM: Failed to map MMIO region at phys=%#p size=%#lux\n", phys_base, tpm_size);
        print("TPM: Using software fallback mode\n");
        tpm_state.initialized = 0;
        tpm_state.base = 0;
        return;
    }

    tpm_state.base = virt_base;
    tpm_state.initialized = 0;  /* Not fully initialized until hardware detected */
    tpm_state.version = 0;

    print("TPM: Mapped MMIO phys=%#p virt=%#p size=%#lux\n", phys_base, (void*)virt_base, tpm_size);

    /* Read Device/Vendor ID */
    did_vid = tpm_read32(TPM_DID_VID_0);

    if(did_vid == 0xFFFFFFFF || did_vid == 0x00000000){
        print("TPM: No TPM hardware detected at %#p (DID/VID: 0x%08X)\n",
              phys_base, did_vid);
        print("TPM: Using software fallback mode\n");
        return;
    }

    print("TPM: Found TPM device (DID/VID: 0x%08X)\n", did_vid);
    print("TPM: Vendor ID: 0x%04X, Device ID: 0x%04X\n",
          did_vid & 0xFFFF, (did_vid >> 16) & 0xFFFF);

    /* Check TPM version via capabilities */
    tpm_state.version = 2;  /* Assume TPM 2.0 */
    tpm_state.initialized = 1;

    print("TPM: TPM 2.0 hardware initialized\n");
}

/*
 * TPM2_GetRandom - Get random bytes from TPM
 */
int
tpm_get_random(u8int *buffer, int len)
{
    u8int cmd[12];
    u8int resp[256];
    usize resp_len = sizeof(resp);
    TPM2_Command_Header *chdr;
    u16int bytes_requested;

    if(!tpm_state.initialized || len <= 0 || len > 64)
        return -1;

    /* Build TPM2_GetRandom command */
    chdr = (TPM2_Command_Header*)cmd;
    chdr->tag = 0x0180;  /* TPM_ST_NO_SESSIONS (big-endian: 0x8001) */
    chdr->size = 0x0C000000;  /* 12 bytes (big-endian) */
    chdr->code = 0x7B010000;  /* TPM2_CC_GetRandom (big-endian: 0x0000017B) */

    bytes_requested = (len >> 8) | (len << 8);  /* Convert to big-endian */
    *(u16int*)(cmd + 10) = bytes_requested;

    /* Send command */
    if(tpm_transmit(0, cmd, 12, resp, &resp_len) < 0)
        return -1;

    /* Parse response: header(10) + size(2) + random_bytes */
    if(resp_len < 12)
        return -1;

    u16int random_size = (resp[10] << 8) | resp[11];
    if(random_size > len || resp_len < 12 + random_size)
        return -1;

    memmove(buffer, resp + 12, random_size);
    return random_size;
}

/*
 * TPM2_PCR_Extend - Extend PCR with hash
 */
int
tpm20_pcr_extend(u32int pcr_handle, u8int *hash, usize hash_len)
{
    u8int cmd[64];
    u8int resp[128];
    usize resp_len = sizeof(resp);
    TPM2_Command_Header *chdr;
    int i;

    if(!tpm_state.initialized || hash_len != 32)
        return -1;

    /* Build TPM2_PCR_Extend command */
    chdr = (TPM2_Command_Header*)cmd;
    chdr->tag = 0x0180;  /* TPM_ST_NO_SESSIONS */
    chdr->size = 0x21000000;  /* 33 bytes base + hash */
    chdr->code = 0x82010000;  /* TPM2_CC_PCR_Extend */

    /* PCR handle (big-endian) */
    i = 10;
    cmd[i++] = (pcr_handle >> 24) & 0xFF;
    cmd[i++] = (pcr_handle >> 16) & 0xFF;
    cmd[i++] = (pcr_handle >> 8) & 0xFF;
    cmd[i++] = pcr_handle & 0xFF;

    /* Auth area size (0 for no sessions) */
    cmd[i++] = 0;
    cmd[i++] = 0;
    cmd[i++] = 0;
    cmd[i++] = 0;

    /* Digest count */
    cmd[i++] = 0;
    cmd[i++] = 1;

    /* Hash algorithm (SHA256 = 0x000B) */
    cmd[i++] = 0x00;
    cmd[i++] = 0x0B;

    /* Hash data */
    memmove(cmd + i, hash, 32);
    i += 32;

    return tpm_transmit(0, cmd, i, resp, &resp_len);
}

/*
 * TPM2_PCR_Read - Read PCR value
 */
int
tpm20_pcr_read(u32int pcr_handle, u8int *pcr_value, usize *pcr_len)
{
    u8int cmd[20];
    u8int resp[256];
    usize resp_len = sizeof(resp);
    TPM2_Command_Header *chdr;

    if(!tpm_state.initialized)
        return -1;

    /* Build TPM2_PCR_Read command */
    chdr = (TPM2_Command_Header*)cmd;
    chdr->tag = 0x0180;
    chdr->size = 0x14000000;  /* 20 bytes */
    chdr->code = 0x7E010000;  /* TPM2_CC_PCR_Read */

    /* PCR selection */
    cmd[10] = 0; cmd[11] = 0; cmd[12] = 0; cmd[13] = 1;  /* Count: 1 */
    cmd[14] = 0x00; cmd[15] = 0x0B;  /* SHA256 */
    cmd[16] = 3;  /* Size of select */
    cmd[17] = (1 << (pcr_handle % 8));  /* PCR bitmap */
    cmd[18] = 0;
    cmd[19] = 0;

    if(tpm_transmit(0, cmd, 20, resp, &resp_len) < 0)
        return -1;

    /* Parse PCR value from response (simplified) */
    if(resp_len > 42 && *pcr_len >= 32){
        memmove(pcr_value, resp + 42, 32);
        *pcr_len = 32;
        return 0;
    }

    return -1;
}

/*
 * tpm_init - Stub for compatibility (tpminit is the real init)
 */
int
tpm_init(void)
{
    /* Already initialized in tpminit() */
    return tpm_state.initialized ? 0 : -1;
}


