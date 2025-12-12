/* TPM Kernel Interface Definitions - Kernel Space */
#ifndef TPM_KERNEL_H
#define TPM_KERNEL_H

#include "u.h"
#include "dat.h"
#include "fns.h"
#include <stdint.h>
#include <stddef.h>

/* Forward declaration - removed, using proper struct definition below */

#include "u.h"
#include "dat.h"
#include "fns.h"

/* TPM 2.0 Command Codes - from Linux kernel */
/* Complete list from include/linux/tpm.h (TPM 2.0 Spec 1.36) */
#define TPM2_CC_FIRST                      0x0000011F
#define TPM2_CC_HIERARCHY_CONTROL          0x00000121
#define TPM2_CC_HIERARCHY_CHANGE_AUTH      0x00000129
#define TPM2_CC_CREATE_PRIMARY             0x00000131
#define TPM2_CC_SEQUENCE_COMPLETE          0x0000013E
#define TPM2_CC_SELF_TEST                  0x00000143
#define TPM2_CC_STARTUP                    0x00000144
#define TPM2_CC_SHUTDOWN                   0x00000145
#define TPM2_CC_NV_READ                    0x0000014E
#define TPM2_CC_CREATE                     0x00000153
#define TPM2_CC_LOAD                       0x00000157
#define TPM2_CC_SEQUENCE_UPDATE            0x0000015C
#define TPM2_CC_UNSEAL                     0x0000015E
#define TPM2_CC_CONTEXT_LOAD               0x00000161
#define TPM2_CC_CONTEXT_SAVE               0x00000162
#define TPM2_CC_FLUSH_CONTEXT              0x00000165
#define TPM2_CC_READ_PUBLIC                0x00000173
#define TPM2_CC_START_AUTH_SESS            0x00000176
#define TPM2_CC_VERIFY_SIGNATURE           0x00000177
#define TPM2_CC_GET_CAPABILITY             0x0000017A
#define TPM2_CC_GET_RANDOM                 0x0000017B
#define TPM2_CC_PCR_READ                   0x0000017E
#define TPM2_CC_PCR_EXTEND                 0x00000182
#define TPM2_CC_EVENT_SEQUENCE_COMPLETE    0x00000185
#define TPM2_CC_HASH_SEQUENCE_START        0x00000186
#define TPM2_CC_CREATE_LOADED              0x00000191
#define TPM2_CC_LAST                       0x00000193

/* Backwards compatibility aliases (deprecated names) */
#define TPM2_CC_GetRandom          TPM2_CC_GET_RANDOM
#define TPM2_CC_CreatePrimary      TPM2_CC_CREATE_PRIMARY
#define TPM2_CC_Unseal             TPM2_CC_UNSEAL
#define TPM2_CC_Startup            TPM2_CC_STARTUP
#define TPM2_CC_SelfTest           TPM2_CC_SELF_TEST
#define TPM2_CC_GetCapability      TPM2_CC_GET_CAPABILITY
#define TPM2_CC_NV_DefineSpace     0x0000012A  /* Not in Linux enum, custom */
#define TPM2_CC_NV_UndefineSpace   0x00000122  /* Not in Linux enum, custom */
#define TPM2_CC_NV_Write           TPM2_CC_NV_READ  /* Placeholder, needs correction */

/* TPM 1.2 Command Codes */
#define TPM_ORD_GetRandom          0x00000046
#define TPM_ORD_OSAP               0x00000017
#define TPM_ORD_DSAP               0x00000018
#define TPM_ORD_MakeIdentity       0x00000029
#define TPM_ORD_ActivateIdentity   0x0000002A
#define TPM_ORD_GetPubKey          0x00000021
#define TPM_ORD_Seal               0x00000017
#define TPM_ORD_UnSeal             0x00000018
#define TPM_ORD_Quote              0x00000022
#define TPM_ORD_Startup            0x00000099
#define TPM_ORD_SelfTestFull       0x00000054
#define TPM_ORD_ContinueSelfTest   0x00000053

/* TPM 1.2 Tags */
#define TPM_TAG_RQU_COMMAND        0x00C1
#define TPM_TAG_RQU_AUTH1_COMMAND  0x00C2
#define TPM_TAG_RQU_AUTH2_COMMAND  0x00C3
#define TPM_TAG_RSP_COMMAND        0x00C4
#define TPM_TAG_RSP_AUTH1_COMMAND  0x00C5
#define TPM_TAG_RSP_AUTH2_COMMAND  0x00C6

/* TPM 2.0 Structure Tags (TPM2_ST) */
#define TPM2_ST_NO_SESSIONS        0x8001
#define TPM2_ST_SESSIONS           0x8002
#define TPM2_ST_CREATION           0x8021

/* TPM 2.0 Startup Types */
#define TPM2_SU_CLEAR              0x0000
#define TPM2_SU_STATE              0x0001

/* TPM 2.0 Return Codes - from Linux kernel */
#define TPM2_RC_SUCCESS            0x0000
#define TPM2_RC_HASH               0x0083  /* RC_FMT1 */
#define TPM2_RC_HANDLE             0x008B
#define TPM2_RC_INTEGRITY          0x009F
#define TPM2_RC_INITIALIZE         0x0100  /* RC_VER1 */
#define TPM2_RC_FAILURE            0x0101
#define TPM2_RC_DISABLED           0x0120
#define TPM2_RC_UPGRADE            0x012D
#define TPM2_RC_COMMAND_CODE       0x0143
#define TPM2_RC_TESTING            0x090A  /* RC_WARN */
#define TPM2_RC_REFERENCE_H0       0x0910
#define TPM2_RC_RETRY              0x0922
#define TPM2_RC_SESSION_MEMORY     0x0903

/* TPM 1.2 Return Codes */
#define TPM_SUCCESS                0x00000000
#define TPM_BAD_TAG                0x0000001E
#define TPM_ECC_PRIVATE_KEY        0x00000091

/* TPM 2.0 Capabilities - from Linux kernel */
#define TPM2_CAP_HANDLES           1
#define TPM2_CAP_COMMANDS          2
#define TPM2_CAP_PCRS              5
#define TPM2_CAP_TPM_PROPERTIES    6

/* TPM 2.0 Permanent Handles */
#define TPM2_RH_NULL               0x40000007
#define TPM2_RS_PW                 0x40000009  /* Password authorization */

/* TPM 2.0 Handle Types (MSO - Most Significant Octet) */
#define TPM2_MSO_NVRAM             0x01
#define TPM2_MSO_SESSION           0x02
#define TPM2_MSO_POLICY            0x03
#define TPM2_MSO_PERMANENT         0x40
#define TPM2_MSO_VOLATILE          0x80
#define TPM2_MSO_PERSISTENT        0x81

/* TPM 1.2 Capability Categories (deprecated) */
#define TPM_CAP_FIRST              0x00000000
#define TPM_CAP_LAST               0xFFFFFFFF
#define TPM_CAP_PCR                0x00000012
#define TPM_CAP_ALG                0x00000003
#define TPM_CAP_HANDLES            TPM2_CAP_HANDLES
#define TPM_CAP_COMMANDS           TPM2_CAP_COMMANDS
#define TPM_CAP_PP_COMMANDS        0x00000003
#define TPM_CAP_AUDIT_COMMANDS     0x00000004
#define TPM_CAP_PCR_PROPERTIES     TPM2_CAP_PCRS

/* TPM 2.0 Algorithm Identifiers - from Linux kernel */
#define TPM_ALG_ERROR              0x0000
#define TPM_ALG_SHA1               0x0004
#define TPM_ALG_AES                0x0006
#define TPM_ALG_KEYEDHASH          0x0008
#define TPM_ALG_SHA256             0x000B
#define TPM_ALG_SHA384             0x000C
#define TPM_ALG_SHA512             0x000D
#define TPM_ALG_NULL               0x0010
#define TPM_ALG_SM3_256            0x0012
#define TPM_ALG_ECC                0x0023
#define TPM_ALG_CFB                0x0043

/* Backwards compatibility aliases */
#define TPM2_ALG_RSA               0x0001  /* Not in Linux enum, custom */
#define TPM2_ALG_KEYEDHASH         TPM_ALG_KEYEDHASH
#define TPM2_ALG_SHA256            TPM_ALG_SHA256
#define TPM2_ALG_SHA1              TPM_ALG_SHA1
#define TPM2_ALG_HMAC              0x0005  /* Not in Linux enum, custom */
#define TPM2_ALG_AES               TPM_ALG_AES

/* TPM Constants */
#define TPM_MAX_HASHES             5

/* TPM Handle Types */
#define TPM2_HR_TRANSIENT          0x80000000
#define TPM2_HR_PERSISTENT         0x81000000
#define TPM2_HR_PERMANENT          0x82000000

/* TPM 1.2 Structures */
typedef struct {
    uint32_t size;
    uint8_t buffer[2048];
} TPM1_2_DIGEST;

typedef struct {
    uint16_t algorithm;
    uint16_t hash;
} TPM1_2_PCR_SELECTION;

/* TPM Context Structure */
typedef struct {
    int version;                    /* TPM_1_2 or TPM_2_0 */
    int fd;                         /* TPM device file descriptor */
    int hardware_available;         /* Hardware vs software fallback */
    uint8_t locality;               /* TPM locality (usually 0) */
    
    /* TPM 1.2 specific */
    struct {
        uint32_t auth_handle;
        uint8_t nonce[20];
        uint8_t even[20];
        uint8_t odd[20];
        uint8_t even_integrity[20];
        uint8_t odd_integrity[20];
    } tpm12;
    
    /* TPM 2.0 specific */
    struct {
        uint32_t session_handle;
        uint8_t salt[32];
    } tpm20;
    
    /* Key handles */
    uint32_t hmac_key_handle;
    uint32_t seal_key_handle;
    uint32_t attestation_key_handle;
} TPMContext;

/* Function Prototypes - Fixed signatures */

/* Core TPM Operations */
int tpm_init(void);
int tpm_startup(TPMContext* ctx);
int tpm_get_random(uint8_t* buffer, int len);

/* TPM 2.0 Operations (tpm2_sapi_minimal.c) */
int tpm2_startup(TPMContext* ctx);
int tpm2_create_primary(TPMContext* ctx, uint32_t *handle_out);
int tpm2_create(TPMContext* ctx, uint32_t parent_handle, uint8_t *data, uint16_t data_len,
                uint8_t *private_out, uint16_t *private_len,
                uint8_t *public_out, uint16_t *public_len);
int tpm2_load(TPMContext* ctx, uint32_t parent_handle, uint8_t *private_blob, uint16_t private_len,
              uint8_t *public_blob, uint16_t public_len, uint32_t *handle_out);
int tpm2_unseal(TPMContext* ctx, uint32_t item_handle, uint8_t *data_out, uint16_t *data_len);

/* TPM 2.0 Management Operations (from Linux kernel) */
int tpm2_flush_context(TPMContext* ctx, uint32_t handle);
int tpm2_shutdown(TPMContext* ctx, uint16_t shutdown_type);
int tpm2_get_capability(TPMContext* ctx, uint32_t capability, uint32_t property, uint32_t *value_out);
int tpm2_self_test(TPMContext* ctx, uint8_t full_test);

/* TPM 2.0 NVRAM Operations */
int tpm2_nv_define_space(TPMContext* ctx, uint32_t nv_index, uint16_t size, uint32_t attributes);
int tpm2_nv_undefine_space(TPMContext* ctx, uint32_t nv_index);
int tpm2_nv_write(TPMContext* ctx, uint32_t nv_index, uint8_t* data, uint16_t len, uint16_t offset);

/* Utility Functions */
int tpm_transmit(TPMContext* ctx, uint8_t* command, size_t cmd_len, uint8_t* response, size_t* resp_len);
void tpm_dump_buffer(const char* prefix, uint8_t* buffer, size_t len);

/* ACPI and Hardware Detection */
typedef struct {
    uint32_t signature;      /* TPM 1.2: 0x41504354 "TPCA", TPM 2.0: 0x544D4152 "TMAT" */
    uint8_t base_address[8]; /* Physical address of TPM base */
    uint32_t start_method;   /* How to start communication */
    uint16_t reserved;
    uint8_t flags;           /* Bit field with options */
    uint8_t init_type;       /* Initialization type */
} ACPI_TPM2_TABLE;

int tpm_detect_acpi(void* acpi_table, uint32_t* base_address, int* version);
int tpm_detect_pci(void);

/* Constants */
#define TPM_1_2                     1
#define TPM_2_0                     2
#define TPM_UNKNOWN                 0

/* Error Codes */
#define TPM_SUCCESS                 0
#define TPM_ERROR                   -1
#define TPM_HARDWARE_NOT_FOUND      -2
#define TPM_DEVICE_OPEN_FAILED      -3
#define TPM_COMMAND_FAILED          -4
#define TPM_TIMEOUT                 -5
#define TPM_BUFFER_TOO_SMALL        -6

#endif /* TPM_KERNEL_H */