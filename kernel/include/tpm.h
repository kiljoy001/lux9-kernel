/* TPM Kernel Interface Definitions - Kernel Space */
#ifndef TPM_KERNEL_H
#define TPM_KERNEL_H

#include "u.h"
#include "dat.h"
#include "fns.h"

/* Forward declaration - removed, using proper struct definition below */

#include "u.h"
#include "dat.h"
#include "fns.h"

/* TPM 2.0 Command Codes */
#define TPM2_CC_GetRandom          0x0000017B
#define TPM2_CC_HMAC               0x00000175
#define TPM2_CC_HMAC_Start         0x00000174
#define TPM2_CC_Hash               0x00000176
#define TPM2_CC_Create             0x00000173
#define TPM2_CC_CreatePrimary      0x00000131
#define TPM2_CC_Load               0x00000137
#define TPM2_CC_Seal               0x00000174
#define TPM2_CC_Unseal             0x0000015E
#define TPM2_CC_Quote              0x00000158
#define TPM2_CC_Startup            0x00000144
#define TPM2_CC_SelfTest           0x00000143
#define TPM2_CC_GetCapability      0x0000011A
#define TPM2_CC_PCR_Read           0x00000117
#define TPM2_CC_PCR_Extend         0x00000118

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

/* TPM Tags */
#define TPM_TAG_RQU_COMMAND        0x00C1
#define TPM_TAG_RQU_AUTH1_COMMAND  0x00C2
#define TPM_TAG_RQU_AUTH2_COMMAND  0x00C3
#define TPM_TAG_RSP_COMMAND        0x00C4
#define TPM_TAG_RSP_AUTH1_COMMAND  0x00C5
#define TPM_TAG_RSP_AUTH2_COMMAND  0x00C6

/* TPM 2.0 Startup Subcommands */
#define TPM2_SU_CLEAR              0x0000
#define TPM2_SU_STATE              0x0001

/* TPM Return Codes */
#define TPM_SUCCESS                0x00000000
#define TPM_BAD_TAG                0x0000001E
#define TPM_ECC_PRIVATE_KEY        0x00000091

/* TPM Capability Categories */
#define TPM_CAP_FIRST              0x00000000
#define TPM_CAP_LAST               0xFFFFFFFF
#define TPM_CAP_PCR                0x00000012
#define TPM_CAP_ALG                0x00000003
#define TPM_CAP_HANDLES            0x00000001
#define TPM_CAP_COMMANDS           0x00000002
#define TPM_CAP_PP_COMMANDS        0x00000003
#define TPM_CAP_AUDIT_COMMANDS     0x00000004
#define TPM_CAP_PCR_PROPERTIES     0x00000012
#define TPM_CAP_LAST               0xFFFFFFFF

/* TPM 2.0 Algorithm Identifiers */
#define TPM2_ALG_RSA               0x0001
#define TPM2_ALG_KEYEDHASH         0x0008
#define TPM2_ALG_SHA256            0x000B
#define TPM2_ALG_SHA1              0x0004
#define TPM2_ALG_HMAC              0x0005
#define TPM2_ALG_AES               0x0006

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
int tpm_init(void);  /* Simplified: no TPMContext parameter */
int tpm_startup(int version, uint16_t mode);
int tpm_self_test(int version);
int tpm_get_random(uint8_t* buffer, int len);
int tpm_get_capability(uint32_t capability, uint32_t property);

/* TPM 1.2 Operations */
int tpm12_osap(TPMContext* ctx, uint16_t key_id, uint8_t* nonce_even);
int tpm12_load_key(TPMContext* ctx, uint32_t parent_handle, uint8_t* key_data, size_t key_size);
int tpm12_quote(TPMContext* ctx, uint32_t key_handle, uint8_t* nonce, uint16_t nonce_size,
                TPM1_2_PCR_SELECTION* pcr_select, uint16_t select_size, uint8_t* quote, size_t* quote_size);

/* TPM 2.0 Operations */
int tpm20_create_primary(TPMContext* ctx, uint32_t primary_handle);
int tpm20_load(TPMContext* ctx, uint32_t parent_handle, uint8_t* public_data, size_t public_size);
int tpm20_hmac(TPMContext* ctx, uint32_t key_handle, uint8_t* data, size_t data_len, uint8_t* hmac_out, size_t* hmac_out_len);
int tpm20_seal(TPMContext* ctx, uint32_t parent_handle, uint8_t* data, size_t data_len, uint8_t* sealed, size_t* sealed_len);
int tpm20_unseal(TPMContext* ctx, uint32_t key_handle, uint8_t* sealed, size_t sealed_len, uint8_t* data, size_t* data_len);
int tpm20_pcr_read(TPMContext* ctx, uint32_t pcr_handle, uint8_t* pcr_value, size_t* pcr_len);
int tpm20_pcr_extend(TPMContext* ctx, uint32_t pcr_handle, uint8_t* hash, size_t hash_len);
int tpm20_quote(TPMContext* ctx, uint32_t signing_key, uint8_t* qualifying_data, size_t qualifying_len,
                uint8_t* signature, size_t* signature_len, uint8_t* quoted, size_t* quoted_len);

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