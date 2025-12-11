/*
 * Minimal TPM2 SAPI/Marshaling - From Scratch
 *
 * Implements only what we need:
 * - TPM2_Create (for sealing)
 * - TPM2_Unseal
 * - Basic marshaling (big-endian encoding)
 *
 * Why from scratch instead of TSS2 library?
 * - TSS2 has 29k LOC with tons of standard library dependencies
 * - We only need ~300 lines for our use case
 * - Simpler to write clean code than fix library conflicts
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/* External TIS driver function */
extern int tpm_transmit(u8int *cmd, usize cmd_len, u8int *resp, usize *resp_len);

/* TPM 2.0 Constants */
#define TPM2_ST_NO_SESSIONS     0x8001
#define TPM2_ST_SESSIONS        0x8002

#define TPM2_CC_Startup         0x00000144
#define TPM2_CC_CreatePrimary   0x00000131
#define TPM2_CC_Create          0x00000153
#define TPM2_CC_Load            0x00000157
#define TPM2_CC_Unseal          0x0000015E
#define TPM2_CC_NV_UndefineSpace 0x00000122
#define TPM2_CC_NV_Write        0x00000137
#define TPM2_CC_NV_DefineSpace  0x0000012A
#define TPM2_CC_HMAC            0x00000172

#define TPM2_SU_CLEAR           0x0000
#define TPM2_SU_STATE           0x0001

#define TPM2_RH_OWNER           0x40000001
#define TPM2_RH_NULL            0x40000007
#define TPM2_RH_PLATFORM        0x4000000C
#define TPM_RS_PW               0x40000009

#define TPM2_ALG_RSA            0x0001
#define TPM2_ALG_SHA256         0x000B
#define TPM2_ALG_KEYEDHASH      0x0008
#define TPM2_ALG_NULL           0x0010
#define TPM2_ALG_ECC            0x0023
#define TPM2_ALG_KDF1_SP800_56A 0x0020
#define TPM2_ECC_NIST_P256      0x0003
#define TPM2_ALG_AES            0x0006
#define TPM2_ALG_CFB            0x0043

#define TPM_SUCCESS             0x00000000

/* Marshaling Helpers - Big Endian */

static void
marshal_u16(u8int **buf, u16int val)
{
	(*buf)[0] = (val >> 8) & 0xFF;
	(*buf)[1] = val & 0xFF;
	*buf += 2;
}

static void
marshal_u32(u8int **buf, u32int val)
{
	(*buf)[0] = (val >> 24) & 0xFF;
	(*buf)[1] = (val >> 16) & 0xFF;
	(*buf)[2] = (val >> 8) & 0xFF;
	(*buf)[3] = val & 0xFF;
	*buf += 4;
}

static u16int
unmarshal_u16(u8int **buf)
{
	u16int val = ((*buf)[0] << 8) | (*buf)[1];
	*buf += 2;
	return val;
}

static u32int
unmarshal_u32(u8int **buf)
{
	u32int val = ((*buf)[0] << 24) | ((*buf)[1] << 16) |
	             ((*buf)[2] << 8) | (*buf)[3];
	*buf += 4;
	return val;
}

/* Marshal TPM2B buffer (size + data) */
static void
marshal_tpm2b(u8int **buf, u8int *data, u16int len)
{
	marshal_u16(buf, len);
	if(len > 0){
		memmove(*buf, data, len);
		*buf += len;
	}
}

/* Unmarshal TPM2B buffer */
static u16int
unmarshal_tpm2b(u8int **buf, u8int *data, u16int max_len)
{
	u16int len = unmarshal_u16(buf);
	if(len > max_len)
		return 0;
	if(len > 0){
		memmove(data, *buf, len);
		*buf += len;
	}
	return len;
}

/*
 * Marshal a Password Session
 * Used for commands requiring authorization (like owner hierarchy access)
 *
 * The authSize is the size of all authorization structures that follow,
 * NOT including the authSize field itself (4 bytes).
 */
static void
marshal_password_session(u8int **buf)
{
	/* Authorization Size (excludes itself, includes session data):
	 * sessionHandle (4) + nonce size (2) + attributes (1) + hmac size (2) = 9 bytes */
	marshal_u32(buf, 9);

	/* Session Data */
	marshal_u32(buf, TPM_RS_PW);    /* sessionHandle: TPM_RS_PW */
	marshal_u16(buf, 0);            /* nonce size: empty */
	*(*buf)++ = 0x00;               /* sessionAttributes: none (continueSession=0) */
	marshal_u16(buf, 0);            /* hmac size: empty (password) */
}

/*
 * TPM2_Startup - Initialize TPM after power-on
 *
 * Must be called before any other TPM commands.
 * Uses TPM2_SU_CLEAR to start with a clean state.
 */
int
tpm2_startup(void)
{
	u8int cmd[12];
	u8int resp[10];
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	/* Command Header */
	marshal_u16(&p, TPM2_ST_NO_SESSIONS);  /* tag */
	marshal_u32(&p, 12);  /* size */
	marshal_u32(&p, TPM2_CC_Startup);  /* command code */

	/* Startup Type */
	marshal_u16(&p, TPM2_SU_CLEAR);  /* Clear state */

	/* Send command */
	if(tpm_transmit(cmd, 12, resp, &resp_len) < 0){
		print("tpm2_startup: transmit failed\n");
		return -1;
	}

	/* Parse response */
	rp = resp;
	unmarshal_u16(&rp);  /* tag */
	unmarshal_u32(&rp);  /* size */
	rc = unmarshal_u32(&rp);  /* response code */

	if(rc != TPM_SUCCESS){
		/* TPM_RC_INITIALIZE (0x100) means already initialized - that's OK */
		if(rc == 0x100){
			print("tpm2_startup: TPM already initialized\n");
			return 0;
		}
		print("tpm2_startup: TPM returned error 0x%08X\n", rc);
		return -1;
	}

	print("tpm2_startup: TPM initialized successfully\n");
	return 0;
}

/*
 * TPM2_CreatePrimary - Create Storage Root Key (SRK)
 *
 * Returns handle to primary key (parent for sealed objects)
 */
int
tpm2_create_primary(u32int *handle_out)
{
	u8int cmd[512];
	u8int resp[512];  /* CreatePrimary response includes full public key */
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	/* Command Header */
	marshal_u16(&p, TPM2_ST_SESSIONS);  /* tag */
	marshal_u32(&p, 0);  /* size - fill later */
	marshal_u32(&p, TPM2_CC_CreatePrimary);  /* command code */

	/* Primary Handle (owner hierarchy) */
	marshal_u32(&p, TPM2_RH_OWNER);

	/* Authorization Session (Password) */
	marshal_password_session(&p);

	/* inSensitive - TPM2B_SENSITIVE_CREATE (empty auth and data) */
	marshal_u16(&p, 4);   /* size of TPMS_SENSITIVE_CREATE = 4 bytes */
	marshal_u16(&p, 0);   /* userAuth size = 0 (no password) */
	marshal_u16(&p, 0);   /* data size = 0 (no sensitive data) */

	/* inPublic - ECC P256 storage key template */
	u8int *public_start = p;
	marshal_u16(&p, 0);  /* size - fill later */

	/* TPMT_PUBLIC for ECC */
	marshal_u16(&p, TPM2_ALG_ECC);  /* type = ECC */
	marshal_u16(&p, TPM2_ALG_SHA256);  /* nameAlg */
	/* objectAttributes: fixedTPM(1) | fixedParent(4) | sensitiveDataOrigin(5) |
	 * userWithAuth(6) | restricted(16) | decrypt(17) = 0x00030072 */
	marshal_u32(&p, 0x00030072);
	marshal_u16(&p, 0);  /* authPolicy size */

	/* TPMS_ECC_PARMS (ECC parameters) */
	/* symmetric: TPMT_SYM_DEF_OBJECT (algorithm, keyBits, mode) */
	marshal_u16(&p, TPM2_ALG_AES);      /* algorithm = AES */
	marshal_u16(&p, 128);               /* keyBits = 128 */
	marshal_u16(&p, TPM2_ALG_CFB);      /* mode = CFB */

	/* TPMT_ECC_SCHEME - NULL for storage key */
	marshal_u16(&p, TPM2_ALG_NULL);  /* scheme */

	/* ECC details */
	marshal_u16(&p, TPM2_ECC_NIST_P256); /* curveID */
	marshal_u16(&p, TPM2_ALG_NULL);  /* kdf */

	/* TPMS_ECC_POINT for unique (X and Y coordinates) */
	marshal_u16(&p, 0);  /* X size - generated by TPM */
	marshal_u16(&p, 0);  /* Y size - generated by TPM */

	/* Fill in public size */
	u16int public_size = p - public_start - 2;
	public_start[0] = (public_size >> 8) & 0xFF;
	public_start[1] = public_size & 0xFF;

	/* outsideInfo (empty) */
	marshal_u16(&p, 0);

	/* creationPCR (empty) */
	marshal_u32(&p, 0);

	/* Fill in command size */
	u32int cmd_size = p - cmd;
	cmd[2] = (cmd_size >> 24) & 0xFF;
	cmd[3] = (cmd_size >> 16) & 0xFF;
	cmd[4] = (cmd_size >> 8) & 0xFF;
	cmd[5] = cmd_size & 0xFF;

	/* Send command */
	if(tpm_transmit(cmd, cmd_size, resp, &resp_len) < 0){
		print("tpm2_create_primary: transmit failed\n");
		return -1;
	}

	/* Parse response */
	rp = resp;
	unmarshal_u16(&rp);  /* tag */
	unmarshal_u32(&rp);  /* size */
	rc = unmarshal_u32(&rp);  /* response code */

	if(rc != TPM_SUCCESS){
		print("tpm2_create_primary: TPM returned error 0x%08X\n", rc);
		return -1;
	}

	/* Extract handle */
	*handle_out = unmarshal_u32(&rp);

	print("tpm2_create_primary: Created SRK with handle 0x%08X\n", *handle_out);
	return 0;
}

/*
 * TPM2_Create - Create sealed data object
 *
 * Seals data under parent key. Data can only be unsealed if PCRs match.
 */
int
tpm2_create(u32int parent_handle, u8int *data, u16int data_len,
            u8int *private_out, u16int *private_len,
            u8int *public_out, u16int *public_len)
{
	u8int cmd[512];
	u8int resp[512];
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	if(data_len > 128){
		print("tpm2_create: data too large (%d bytes, max 128)\n", data_len);
		return -1;
	}

	/* Command Header */
	marshal_u16(&p, TPM2_ST_SESSIONS);
	marshal_u32(&p, 0);
	marshal_u32(&p, TPM2_CC_Create);

	/* Parent handle */
	marshal_u32(&p, parent_handle);

	/* Authorization Session (Password) for parent */
	marshal_password_session(&p);

	/* inSensitive - contains the data to seal */
	u8int *sens_start = p;
	marshal_u16(&p, 0);  /* size - fill later */
	marshal_u16(&p, 0);  /* userAuth size (no password) */
	marshal_tpm2b(&p, data, data_len);  /* sensitive data */

	u16int sens_size = p - sens_start - 2;
	sens_start[0] = (sens_size >> 8) & 0xFF;
	sens_start[1] = sens_size & 0xFF;

	/* inPublic - keyedhash object (sealed data) */
	u8int *public_start = p;
	marshal_u16(&p, 0);  /* size - fill later */

	marshal_u16(&p, TPM2_ALG_KEYEDHASH);  /* type */
	marshal_u16(&p, TPM2_ALG_SHA256);  /* nameAlg */
	/* objectAttributes: fixedTPM(1) | fixedParent(4) | userWithAuth(6) = 0x00000052
	 * NOTE: sensitiveDataOrigin MUST be 0 when sealing user-provided data.
	 * Setting it to 1 tells TPM to generate its own random data and ignore inSensitive.data */
	marshal_u32(&p, 0x00000052);
	marshal_u16(&p, 0);  /* authPolicy size = 0 (use password auth, not policy) */

	/* KEYEDHASH parameters */
	marshal_u16(&p, TPM2_ALG_NULL);  /* scheme */
	marshal_u16(&p, 0);  /* unique size */

	u16int public_size = p - public_start - 2;
	public_start[0] = (public_size >> 8) & 0xFF;
	public_start[1] = public_size & 0xFF;

	/* outsideInfo (empty) */
	marshal_u16(&p, 0);

	/* creationPCR (empty - not sealing to PCRs yet) */
	marshal_u32(&p, 0);

	/* Fill command size */
	u32int cmd_size = p - cmd;
	cmd[2] = (cmd_size >> 24) & 0xFF;
	cmd[3] = (cmd_size >> 16) & 0xFF;
	cmd[4] = (cmd_size >> 8) & 0xFF;
	cmd[5] = cmd_size & 0xFF;

	/* Send command */
	if(tpm_transmit(cmd, cmd_size, resp, &resp_len) < 0){
		print("tpm2_create: transmit failed\n");
		return -1;
	}

	/* Parse response */
	rp = resp;
	unmarshal_u16(&rp);  /* tag */
	unmarshal_u32(&rp);  /* size */
	rc = unmarshal_u32(&rp);

	if(rc != TPM_SUCCESS){
		print("tpm2_create: TPM returned error 0x%08X\n", rc);
		return -1;
	}

	/* Extract private and public blobs */
	/* Note: Response includes parameter size (U32) before parameters? No, only for sessions?
	 * TPM2_Create Response:
	 *   outPrivate (2B)
	 *   outPublic (2B)
	 *   creationData (2B)
	 *   creationHash (2B)
	 *   creationTicket (TK)
	 *
	 * BUT, since we sent a session, the response MAY contain a session tag?
	 * If response code is SUCCESS, the tag is TPM_ST_SESSIONS?
	 * No, response tag matches if sessions are present in response.
	 * We are not requesting a session in response (sessionAttributes=0).
	 * However, TPM might return a session. We'll ignore it if we can.
	 *
	 * Actually, standard TIS response parsing in tpm2_driver.c blindly reads response.
	 * If TPM returns sessions, the 'size' field covers it.
	 * But we need to know WHERE the parameters are.
	 * If tag == TPM_ST_SESSIONS, then parameterSize (U32) follows commandCode.
	 *
	 * Let's check tag.
	 */
	u16int rtag = (resp[0] << 8) | resp[1];
	if(rtag == TPM2_ST_SESSIONS){
		/* Skip parameter size */
		unmarshal_u32(&rp);
	}

	*private_len = unmarshal_tpm2b(&rp, private_out, 256);
	*public_len = unmarshal_tpm2b(&rp, public_out, 256);

	print("tpm2_create: Created sealed object (priv=%d, pub=%d bytes)\n",
	      *private_len, *public_len);
	return 0;
}

/*
 * TPM2_Load - Load sealed object into TPM
 *
 * Returns handle to loaded object
 */
int
tpm2_load(u32int parent_handle, u8int *private_blob, u16int private_len,
          u8int *public_blob, u16int public_len, u32int *handle_out)
{
	u8int cmd[512];
	u8int resp[256];
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	/* Command Header */
	marshal_u16(&p, TPM2_ST_SESSIONS);
	marshal_u32(&p, 0);  /* size - fill later */
	marshal_u32(&p, TPM2_CC_Load);

	/* Parent handle */
	marshal_u32(&p, parent_handle);

	/* Authorization Session (Password) for parent */
	marshal_password_session(&p);

	/* Private blob */
	marshal_tpm2b(&p, private_blob, private_len);

	/* Public blob */
	marshal_tpm2b(&p, public_blob, public_len);

	/* Fill command size */
	u32int cmd_size = p - cmd;
	cmd[2] = (cmd_size >> 24) & 0xFF;
	cmd[3] = (cmd_size >> 16) & 0xFF;
	cmd[4] = (cmd_size >> 8) & 0xFF;
	cmd[5] = cmd_size & 0xFF;

	/* Send command */
	if(tpm_transmit(cmd, cmd_size, resp, &resp_len) < 0){
		print("tpm2_load: transmit failed\n");
		return -1;
	}

	/* Parse response */
	rp = resp;
	u16int rtag = unmarshal_u16(&rp);  /* tag */
	unmarshal_u32(&rp);  /* size */
	rc = unmarshal_u32(&rp);

	if(rc != TPM_SUCCESS){
		print("tpm2_load: TPM returned error 0x%08X\n", rc);
		return -1;
	}

	if(rtag == TPM2_ST_SESSIONS){
		unmarshal_u32(&rp);
	}

	/* Extract handle */
	*handle_out = unmarshal_u32(&rp);

	print("tpm2_load: Loaded object with handle 0x%08X\n", *handle_out);
	return 0;
}

/*
 * TPM2_Unseal - Unseal data from loaded object
 */
int
tpm2_unseal(u32int item_handle, u8int *data_out, u16int *data_len)
{
	u8int cmd[128];
	u8int resp[256];
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	/* Command Header */
	marshal_u16(&p, TPM2_ST_SESSIONS);
	marshal_u32(&p, 0);  /* size - fill later */
	marshal_u32(&p, TPM2_CC_Unseal);

	/* Item handle */
	marshal_u32(&p, item_handle);

	/* Authorization Session (Password) for item */
	marshal_password_session(&p);

	/* Fill command size */
	u32int cmd_size = p - cmd;
	cmd[2] = (cmd_size >> 24) & 0xFF;
	cmd[3] = (cmd_size >> 16) & 0xFF;
	cmd[4] = (cmd_size >> 8) & 0xFF;
	cmd[5] = cmd_size & 0xFF;

	/* Send command */
	if(tpm_transmit(cmd, cmd_size, resp, &resp_len) < 0){
		print("tpm2_unseal: transmit failed\n");
		return -1;
	}

	/* Parse response */
	rp = resp;
	u16int rtag = unmarshal_u16(&rp);  /* tag */
	unmarshal_u32(&rp);  /* size */
	rc = unmarshal_u32(&rp);

	if(rc != TPM_SUCCESS){
		print("tpm2_unseal: TPM returned error 0x%08X\n", rc);
		return -1;
	}

	if(rtag == TPM2_ST_SESSIONS){
		unmarshal_u32(&rp);
	}

	/* Skip parameter size (if present in unseal response? Unseal returns 'outData')
	 * Unseal response: outData (2B)
	 * It IS a parameter.
	 */
	/* Wait, if sessions are present, the parameterSize field TELLS us the size of parameters.
	 * But we usually skip it to find parameters.
	 * If NO sessions, parameters are immediate.
	 * If SESSIONS, parameterSize is U32 before parameters.
	 */
	/* In previous code we skipped parameter size unconditionally? No.
	 * Let's be careful.
	 * If tag == SESSIONS, we skipped 4 bytes (parameterSize) above.
	 * That puts us at start of parameters.
	 * So we are good.
	 */

	/* Extract unsealed data */
	*data_len = unmarshal_tpm2b(&rp, data_out, 128);

	print("tpm2_unseal: Unsealed %d bytes\n", *data_len);
	return 0;
}

/*
 * TPM2_NV_DefineSpace - Define NVRAM index
 */
int
tpm2_nv_define_space(u32int nv_index, u16int size, u32int attributes)
{
	u8int cmd[256];
	u8int resp[64];
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	/* Command Header */
	marshal_u16(&p, TPM2_ST_SESSIONS);
	marshal_u32(&p, 0);  /* size - fill later */
	marshal_u32(&p, TPM2_CC_NV_DefineSpace);

	/* Authorization Handle (Owner or Platform) */
	marshal_u32(&p, TPM2_RH_OWNER);

	/* Authorization Session (Password) */
	marshal_password_session(&p);

	/* Auth (empty password for the index) */
	marshal_u16(&p, 0);  /* size */

	/* TPMS_NV_PUBLIC */
	marshal_u16(&p, 0);  /* size of TPMS_NV_PUBLIC - fill later */
	u8int *public_start = p;

	marshal_u32(&p, nv_index);  /* nvIndex */
	marshal_u16(&p, TPM2_ALG_SHA256);  /* nameAlg */
	marshal_u32(&p, attributes);  /* attributes (e.g. TPMA_NV_OWNERWRITE) */
	marshal_u16(&p, 0);  /* authPolicy size */
	marshal_u16(&p, size);  /* dataSize */

	/* Fill TPMS_NV_PUBLIC size */
	u16int public_size = p - public_start;
	public_start[-2] = (public_size >> 8) & 0xFF;
	public_start[-1] = public_size & 0xFF;

	/* Fill command size */
	u32int cmd_size = p - cmd;
	cmd[2] = (cmd_size >> 24) & 0xFF;
	cmd[3] = (cmd_size >> 16) & 0xFF;
	cmd[4] = (cmd_size >> 8) & 0xFF;
	cmd[5] = cmd_size & 0xFF;

	if(tpm_transmit(cmd, cmd_size, resp, &resp_len) < 0){
		print("tpm2_nv_define_space: transmit failed\n");
		return -1;
	}

	rp = resp;
	unmarshal_u16(&rp);
	unmarshal_u32(&rp);
	rc = unmarshal_u32(&rp);

	if(rc != TPM_SUCCESS){
		print("tpm2_nv_define_space: TPM error 0x%08X\n", rc);
		return -1;
	}

	return 0;
}

/*
 * TPM2_NV_UndefineSpace - Delete NVRAM index
 */
int
tpm2_nv_undefine_space(u32int nv_index)
{
	u8int cmd[128];
	u8int resp[64];
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	/* Command Header */
	marshal_u16(&p, TPM2_ST_SESSIONS);
	marshal_u32(&p, 0);
	marshal_u32(&p, TPM2_CC_NV_UndefineSpace);

	/* Authorization Handle (Owner or Platform) */
	marshal_u32(&p, TPM2_RH_OWNER);

	/* NV Index */
	marshal_u32(&p, nv_index);

	/* Authorization Session (Password) */
	marshal_password_session(&p);

	/* Fill command size */
	u32int cmd_size = p - cmd;
	cmd[2] = (cmd_size >> 24) & 0xFF;
	cmd[3] = (cmd_size >> 16) & 0xFF;
	cmd[4] = (cmd_size >> 8) & 0xFF;
	cmd[5] = cmd_size & 0xFF;

	if(tpm_transmit(cmd, cmd_size, resp, &resp_len) < 0){
		print("tpm2_nv_undefine_space: transmit failed\n");
		return -1;
	}

	rp = resp;
	unmarshal_u16(&rp);
	unmarshal_u32(&rp);
	rc = unmarshal_u32(&rp);

	if(rc != TPM_SUCCESS){
		print("tpm2_nv_undefine_space: TPM error 0x%08X\n", rc);
		return -1;
	}

	return 0;
}

/*
 * TPM2_NV_Write - Write data to NVRAM
 */
int
tpm2_nv_write(u32int nv_index, u8int *data, u16int len, u16int offset)
{
	u8int cmd[1024];  /* Max NV write is small usually, but buffer needs space */
	u8int resp[64];
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	if(len > 1024) return -1;

	/* Command Header */
	marshal_u16(&p, TPM2_ST_SESSIONS);
	marshal_u32(&p, 0);
	marshal_u32(&p, TPM2_CC_NV_Write);

	/* Auth Handle (Owner) */
	marshal_u32(&p, TPM2_RH_OWNER);

	/* NV Index */
	marshal_u32(&p, nv_index);

	/* Authorization Session (Password) */
	marshal_password_session(&p);

	/* Data */
	marshal_tpm2b(&p, data, len);

	/* Offset */
	marshal_u16(&p, offset);

	/* Fill command size */
	u32int cmd_size = p - cmd;
	cmd[2] = (cmd_size >> 24) & 0xFF;
	cmd[3] = (cmd_size >> 16) & 0xFF;
	cmd[4] = (cmd_size >> 8) & 0xFF;
	cmd[5] = cmd_size & 0xFF;

	if(tpm_transmit(cmd, cmd_size, resp, &resp_len) < 0){
		print("tpm2_nv_write: transmit failed\n");
		return -1;
	}

	rp = resp;
	unmarshal_u16(&rp);
	unmarshal_u32(&rp);
	rc = unmarshal_u32(&rp);

	if(rc != TPM_SUCCESS){
		print("tpm2_nv_write: TPM error 0x%08X\n", rc);
		return -1;
	}

	return 0;
}

/*
 * TPM2_HMAC - Compute HMAC using key in TPM
 */
int
tpm20_hmac(u32int key_handle, u8int *data, usize data_len, u8int *hmac_out, usize *hmac_out_len)
{
	u8int cmd[1024];
	u8int resp[1024];
	usize resp_len = sizeof(resp);
	u8int *p = cmd;
	u8int *rp;
	u32int rc;

	if(data_len > 1024) return -1;

	/* Command Header */
	marshal_u16(&p, TPM2_ST_SESSIONS);
	marshal_u32(&p, 0);
	marshal_u32(&p, TPM2_CC_HMAC);

	/* Key Handle */
	marshal_u32(&p, key_handle);

	/* Authorization Session (Password) */
	marshal_password_session(&p);

	/* Buffer */
	marshal_tpm2b(&p, data, data_len);

	/* Hash Algorithm */
	marshal_u16(&p, TPM2_ALG_SHA256);

	/* Fill command size */
	u32int cmd_size = p - cmd;
	cmd[2] = (cmd_size >> 24) & 0xFF;
	cmd[3] = (cmd_size >> 16) & 0xFF;
	cmd[4] = (cmd_size >> 8) & 0xFF;
	cmd[5] = cmd_size & 0xFF;

	if(tpm_transmit(cmd, cmd_size, resp, &resp_len) < 0){
		print("tpm20_hmac: transmit failed\n");
		return -1;
	}

	rp = resp;
	u16int rtag = unmarshal_u16(&rp);
	unmarshal_u32(&rp);
	rc = unmarshal_u32(&rp);

	if(rc != TPM_SUCCESS){
		print("tpm20_hmac: TPM error 0x%08X\n", rc);
		return -1;
	}

	if(rtag == TPM2_ST_SESSIONS){
		unmarshal_u32(&rp);
	}

	/* Extract digest */
	*hmac_out_len = unmarshal_tpm2b(&rp, hmac_out, 64);

	return 0;
}