/* fruity_opcodes.h - Fruity IR Opcode Definitions
 *
 * The "Fruity Flavors" - Opcodes mapped to Pebble colors and operations.
 * Each opcode corresponds to explicit Pebble memory management primitives.
 *
 * Design Philosophy:
 *   - LIME/VANILLA/BURN: Core reference counting via white tokens
 *   - CHERRY/BERRY/ROLLBACK: Transactional memory (Red-Blue shadows)
 *   - GRAPE/LEMON: Zero-copy ownership transfer
 *   - Standard control flow: CALL, RET, JUMP, BRANCH
 */

#ifndef FRUITY_OPCODES_H
#define FRUITY_OPCODES_H

/* Fruity IR Opcode enumeration
 *
 * Opcodes are organized into categories:
 *   0x000-0x0FF: Standard operations (NOP, arithmetic, etc.)
 *   0x100-0x1FF: Pebble memory operations (LIME, VANILLA, BURN)
 *   0x200-0x2FF: Transactional operations (CHERRY, BERRY, ROLLBACK)
 *   0x300-0x3FF: IPC and ownership transfer (GRAPE, LEMON)
 *   0x400-0x4FF: Control flow (CALL, RET, JUMP, BRANCH)
 *   0x500-0x5FF: Stack and local operations
 */
#define FRUITY_BEQ 0x410
#define FRUITY_BNE 0x411
#define FRUITY_BLT 0x412
#define FRUITY_BLE 0x413
#define FRUITY_BGT 0x414
#define FRUITY_BGE 0x415
#define FRUITY_BTRUE 0x416
#define FRUITY_BFALSE 0x417

typedef enum {
  /* ===== Standard Operations (0x000-0x0FF) ===== */
  FRUITY_NOP = 0x000,   /* No operation */
  FRUITY_BREAK = 0x001, /* Debugger break */

  /* Arithmetic */
  FRUITY_ADD = 0x010,     /* Add two values */
  FRUITY_SUB = 0x011,     /* Subtract */
  FRUITY_MUL = 0x012,     /* Multiply */
  FRUITY_DIV = 0x013,     /* Divide */
  FRUITY_REM = 0x014,     /* Remainder */
  FRUITY_NEG = 0x015,     /* Negate */
  FRUITY_DIV_UN = 0x016,  /* Divide unsigned */
  FRUITY_REM_UN = 0x017,  /* Remainder unsigned */
  FRUITY_ADD_OVF = 0x018, /* Add check overflow */
  FRUITY_ADD_OVF_UN = 0x019,
  FRUITY_MUL_OVF = 0x01A, /* Mul check overflow */
  FRUITY_MUL_OVF_UN = 0x01B,
  FRUITY_SUB_OVF = 0x01C,
  FRUITY_SUB_OVF_UN = 0x01D,

  /* Bitwise */
  FRUITY_AND = 0x020,    /* Bitwise AND */
  FRUITY_OR = 0x021,     /* Bitwise OR */
  FRUITY_XOR = 0x022,    /* Bitwise XOR */
  FRUITY_NOT = 0x023,    /* Bitwise NOT */
  FRUITY_SHL = 0x024,    /* Shift left */
  FRUITY_SHR = 0x025,    /* Shift right */
  FRUITY_SHR_UN = 0x026, /* Shift right unsigned */

  /* Comparison */
  FRUITY_CEQ = 0x030, /* Compare equal */
  FRUITY_CNE = 0x031, /* Compare not equal */
  FRUITY_CLT = 0x032, /* Compare less than */
  FRUITY_CLE = 0x033, /* Compare less or equal */
  FRUITY_CGT = 0x034, /* Compare greater than */
  FRUITY_CGE = 0x035, /* Compare greater or equal */

  /* Constants */
  FRUITY_LDC_I4 = 0x040, /* Load int32 constant */
  FRUITY_LDC_I8 = 0x041, /* Load int64 constant */
  FRUITY_LDC_R4 = 0x042, /* Load float32 constant */
  FRUITY_LDC_R8 = 0x043, /* Load float64 constant */
  FRUITY_LDNULL = 0x044, /* Load null reference */

  /* Conversion */
  FRUITY_CONV_I4 = 0x050, /* Convert to int32 */
  FRUITY_CONV_I8 = 0x051, /* Convert to int64 */
  FRUITY_CONV_R4 = 0x052, /* Convert to float32 */
  FRUITY_CONV_R8 = 0x053, /* Convert to float64 */

  /* ===== Pebble Memory Operations (0x100-0x1FF) ===== */

  /* LIME - Allocate (Black Pebble + first White Token)
   * Maps to: clr_object_alloc()
   * Source: MSIL 'newobj'
   * Effect: Creates Black Pebble, issues first White Token
   * Stack: size → obj_ref
   */
  FRUITY_LIME = 0x100,

  /* VANILLA - Share reference (Issue White Token)
   * Maps to: clr_object_addref()
   * Source: MSIL 'dup' on ref, 'ldloc' on ref
   * Effect: Issues new White Token, increments white_count
   * Stack: obj_ref → obj_ref, obj_ref
   */
  FRUITY_VANILLA = 0x101,

  /* BURN - Release reference (Burn White Token)
   * Maps to: clr_object_release()
   * Source: MSIL 'pop' on ref, 'stloc' overwrite
   * Effect: Burns White Token, decrements white_count, frees if 0
   * Stack: obj_ref → ∅
   */
  FRUITY_BURN = 0x102,

  /* DUP - Duplicate top of stack
   * For value types: simple copy
   * For reference types: VANILLA (issue white token)
   * Stack: value → value, value
   */
  FRUITY_DUP = 0x103,

  /* POP - Remove top of stack
   * For value types: simple removal
   * For reference types: BURN (release white token)
   * Stack: value → ∅
   */
  FRUITY_POP = 0x104,

  /* LOAD_STRING - Load string literal
   * Maps to: clr_string_from_literal()
   * Source: MSIL 'ldstr'
   * Effect: Returns managed string object (White Token)
   * Stack: → string_ref
   */
  FRUITY_LOAD_STRING = 0x105,

  /* ===== Transactional Operations (0x200-0x2FF) ===== */

  /* CHERRY - Create Red snapshot (Begin transaction)
   * Maps to: clr_object_snapshot()
   * Source: [Transactional] method entry
   * Effect: Creates Red shadow for rollback
   * Stack: obj_ref → obj_ref (now has Red shadow)
   */
  FRUITY_CHERRY = 0x200,

  /* BERRY - Commit transaction (Keep Blue changes)
   * Maps to: clr_object_commit()
   * Source: [Transactional] method success return
   * Effect: Discards Red shadow, keeps Blue changes
   * Stack: obj_ref → obj_ref (Red discarded)
   */
  FRUITY_BERRY = 0x201,

  /* ROLLBACK - Abort transaction (Restore from Red)
   * Maps to: clr_object_rollback()
   * Source: [Transactional] exception handler
   * Effect: Discards Blue changes, restores from Red
   * Stack: obj_ref → obj_ref (Blue discarded, Red restored)
   */
  FRUITY_ROLLBACK = 0x202,

  /* ===== IPC and Ownership Transfer (0x300-0x3FF) ===== */

  /* GRAPE - Zero-copy IPC transfer (Exchange)
   * Maps to: clr_msg_prepare() + clr_msg_send()
   * Source: [Exchange] attributed method call
   * Effect: Transfers White Token to another process/tasklet
   * Stack: obj_ref, dest_tasklet → ∅
   * Note: obj_ref becomes invalid in caller after this
   */
  FRUITY_GRAPE = 0x300,

  /* LEMON - Intra-process ownership transfer (Move)
   * Maps to: White Token handover without addref/release
   * Source: 'ret' with ref type, or explicit move
   * Effect: Transfers ownership without changing white_count
   * Stack: obj_ref_src → obj_ref_dst
   */
  FRUITY_LEMON = 0x301,

  /* ===== Control Flow (0x400-0x4FF) ===== */

  /* CALL - Function call
   * Stack: arg1, arg2, ..., argN → result
   */
  FRUITY_CALL = 0x400,

  /* CALLI - Indirect function call
   * Source: MSIL 'calli'
   * Stack: arg1, arg2, ..., argN, fn_ptr → result
   */
  FRUITY_CALLI = 0x401,

  /* RET - Return from function
   * Stack: result → (caller stack)
   */
  FRUITY_RET = 0x402,

  /* JUMP - Unconditional branch
   * Source: MSIL 'br', 'br.s'
   * Stack: (unchanged)
   */
  FRUITY_JUMP = 0x403,

  /* SWITCH - Jump table branch
   * Source: MSIL 'switch'
   * Operand: immediate int32 (number of targets)
   * Stack: index → (branch)
   * Note: Targets are stored in subsequent data/instructions
   */
  FRUITY_SWITCH = 0x404,

  /* LDFTN - Load function pointer
   * Source: MSIL 'ldftn'
   * Stack: → fn_ptr
   */
  FRUITY_LDFTN = 0x405,

  /* LDVIRTFTN - Load virtual function pointer
   * Source: MSIL 'ldvirtftn'
   * Stack: obj_ref → fn_ptr
   */
  FRUITY_LDVIRTFTN = 0x406,

  /* Branch instructions (conditional jumps) */

  /* ===== Stack and Local Operations (0x500-0x5FF) ===== */

  /* LOAD_LOCAL - Load value from local variable
   * Source: MSIL 'ldloc', 'ldloc.s', 'ldloc.0'...'ldloc.3'
   * Stack: → value
   */
  FRUITY_LOAD_LOCAL = 0x510,

  /* LOAD_LOCAL_ADDR - Load address of local variable
   * Source: MSIL 'ldloca', 'ldloca.s'
   * Stack: → &local
   */
  FRUITY_LOAD_LOCAL_ADDR = 0x511,

  /* STORE_LOCAL - Store value to local variable
   * Source: MSIL 'stloc', 'stloc.s', 'stloc.0'...'stloc.3'
   * Stack: value → ∅
   */
  FRUITY_STORE_LOCAL = 0x512,

  /* LOAD_ARG - Load argument value
   * Source: MSIL 'ldarg', 'ldarg.s', 'ldarg.0'...'ldarg.3'
   * Stack: → value
   */
  FRUITY_LOAD_ARG = 0x513,

  /* LOAD_ARG_ADDR - Load argument address
   * Source: MSIL 'ldarga', 'ldarga.s'
   * Stack: → &arg
   */
  FRUITY_LOAD_ARG_ADDR = 0x514,

  /* STORE_ARG - Store value to argument
   * Source: MSIL 'starg', 'starg.s'
   * Stack: value → ∅
   */
  FRUITY_STORE_ARG = 0x515,

  /* LOAD_FIELD - Load field from object
   * Source: MSIL 'ldfld'
   * Stack: obj_ref → value
   */
  FRUITY_LOAD_FIELD = 0x520,

  /* STORE_FIELD - Store value to object field
   * Source: MSIL 'stfld'
   * Stack: obj_ref, value → ∅
   */
  FRUITY_STORE_FIELD = 0x521,

  /* LOAD_STATIC - Load static field
   * Source: MSIL 'ldsfld'
   * Stack: → value
   */
  FRUITY_LOAD_STATIC = 0x522,

  /* STORE_STATIC - Store static field
   * Source: MSIL 'stsfld'
   * Stack: value → ∅
   */
  FRUITY_STORE_STATIC = 0x523,

  /* LOAD_FIELD_ADDR - Load address of object field
   * Stack: obj_ref, field_offset → addr
   */
  FRUITY_LDFLDA = 0x50A,

  /* LOAD_IND - Load indirect from address
   * Maps to: *ptr
   * Source: MSIL 'ldind.*'
   * Effect: Loads value from address
   * Stack: ptr → value
   */
  FRUITY_LOAD_IND = 0x506,

  /* STORE_IND - Store indirect to address
   * Maps to: *ptr = val
   * Source: MSIL 'stind.*'
   * Effect: Stores value to address
   * Stack: ptr, value → ∅
   */
  FRUITY_STORE_IND = 0x507,

  /* MEMCPY - Copy block of memory
   * Maps to: memcpy(dest, src, size)
   * Source: MSIL 'cpblk'
   * Stack: dest, src, size → ∅
   */
  FRUITY_MEMCPY = 0x508,

  /* MEMSET - Initialize block of memory
   * Maps to: memset(dest, val, size)
   * Source: MSIL 'initblk'
   * Stack: dest, val, size → ∅
   */
  FRUITY_MEMSET = 0x509,

  /* ===== Object Model Operations (0x600-0x6FF) ===== */

  /* CASTCLASS - Cast object to type
   * Maps to: runtime type check
   * Source: MSIL 'castclass'
   * Effect: Checks type, returns object or throws InvalidCastException
   * Stack: obj_ref → obj_ref
   */
  FRUITY_CASTCLASS = 0x600,

  /* ISINST - Type check
   * Maps to: runtime type check
   * Source: MSIL 'isinst'
   * Effect: Checks type, returns object or null
   * Stack: obj_ref → obj_ref (or null)
   */
  FRUITY_ISINST = 0x601,

  /* BOX - Box value type
   * Maps to: clr_box()
   * Source: MSIL 'box'
   * Effect: Allocates object (LIME), copies value
   * Stack: value → obj_ref
   */
  FRUITY_BOX = 0x602,

  /* UNBOX - Get address of value in boxed object
   * Maps to: clr_unbox()
   * Source: MSIL 'unbox'
   * Effect: Returns managed pointer to value
   * Stack: obj_ref → managed_ptr
   */
  FRUITY_UNBOX = 0x603,

  /* UNBOX_ANY - Unbox value to stack
   * Maps to: clr_unbox_any()
   * Source: MSIL 'unbox.any'
   * Effect: Unboxes value type to stack
   * Stack: obj_ref → value
   */
  FRUITY_UNBOX_ANY = 0x604,

  /* INITOBJ - Initialize value type at address
   * Maps to: memset(0) or constructor
   * Source: MSIL 'initobj'
   * Effect: Initializes memory
   * Stack: dest_ptr → ∅
   */
  FRUITY_INITOBJ = 0x605,

  /* CPOBJ - Copy value type
   * Maps to: memcpy()
   * Source: MSIL 'cpobj'
   * Effect: Copies value from source to dest
   * Stack: dest_ptr, src_ptr → ∅
   */
  FRUITY_CPOBJ = 0x606,

  /* LDOBJ - Load value type from address
   * Maps to: memcpy() to stack
   * Source: MSIL 'ldobj'
   * Effect: Loads value from address
   * Stack: src_ptr → value
   */
  FRUITY_LDOBJ = 0x607,

  /* STOBJ - Store value type to address
   * Maps to: memcpy() from stack
   * Source: MSIL 'stobj'
   * Effect: Stores value to address
   * Stack: dest_ptr, value → ∅
   */
  FRUITY_STOBJ = 0x608,

  /* NEWOBJ - Create new object
   * Maps to: clr_newobj()
   * Source: MSIL 'newobj'
   * Effect: Allocates object, calls constructor
   * Stack: args... → obj_ref
   */
  FRUITY_NEWOBJ = 0x609,

  /* ===== Typed Reference Operations (0x680-0x6FF) ===== */

  /* MKREFANY - Make typed reference
   * Source: MSIL 'mkrefany'
   * Stack: ptr, type_token → typed_ref
   */
  FRUITY_MKREFANY = 0x680,

  /* REFANYVAL - Get value from typed reference
   * Source: MSIL 'refanyval'
   * Stack: typed_ref, type_token → ptr
   */
  FRUITY_REFANYVAL = 0x681,

  /* REFANYTYPE - Get type from typed reference
   * Source: MSIL 'refanytype'
   * Stack: typed_ref → type_token
   */
  FRUITY_REFANYTYPE = 0x682,

  /* ===== Exception Handling (0x700-0x7FF) ===== */

  /* THROW - Throw exception
   * Maps to: runtime exception dispatch
   * Source: MSIL 'throw'
   * Effect: Unwinds stack, dispatches to handler
   * Stack: exception_ref → (unwound)
   */
  FRUITY_THROW = 0x700,

  /* RETHROW - Rethrow current exception
   * Maps to: runtime exception rethrow
   * Source: MSIL 'rethrow'
   * Effect: Re-dispatches current exception
   * Stack: → (unwound)
   */
  FRUITY_RETHROW = 0x701,

  /* LEAVE - Leave protected region
   * Maps to: exit try/catch block
   * Source: MSIL 'leave', 'leave.s'
   * Effect: Cleans up and branches to target
   * Stack: (cleared) → jump to target
   */
  FRUITY_LEAVE = 0x702,

  /* ENDFINALLY - End finally/fault block
   * Maps to: runtime finally cleanup
   * Source: MSIL 'endfinally', 'endfault'
   * Effect: Resumes exception dispatch or normal flow
   * Stack: → (continues exception or returns)
   */
  FRUITY_ENDFINALLY = 0x703,

  /* ENDFILTER - End exception filter
   * Maps to: filter result evaluation
   * Source: MSIL 'endfilter'
   * Effect: Returns filter result (0=reject, 1=accept)
   * Stack: int32 → (filter evaluated)
   */
  /* ===== Prefixes (0xB00-0xBFF) ===== */
  FRUITY_PREFIX_CONSTRAINED = 0xB00, /* constrained. */
  FRUITY_PREFIX_READONLY = 0xB01,    /* readonly. */
  FRUITY_PREFIX_NO = 0xB02,          /* no. */
  FRUITY_PREFIX_TAIL = 0xB03,        /* tail. */
  FRUITY_PREFIX_UNALIGNED = 0xB04,   /* unaligned. */
  FRUITY_PREFIX_VOLATILE = 0xB05,    /* volatile. */

  /* ===== Array Operations (0x800-0x8FF) ===== */

  FRUITY_NEWARR = 0x800, /* New array */
  FRUITY_LDLEN = 0x801,  /* Load array length */

  /* ===== Metadata & Reflection (0x900-0x9FF) ===== */
  FRUITY_SIZEOF = 0x900,  /* Size of type */
  FRUITY_LDTOKEN = 0x901, /* Load metadata token handle */
  FRUITY_ARGLIST = 0x902, /* Load argument iterator */
  FRUITY_JMP = 0x903,     /* Jump to method */

  /* ===== Safety Checks (0xA00-0xAFF) ===== */
  FRUITY_CKFINITE = 0xA00, /* Check finite float */
  FRUITY_LDELEM = 0x802,   /* Load array element */
  FRUITY_STELEM = 0x803,   /* Store array element */
  FRUITY_LDELEMA = 0x804,  /* Load element address */

} fruity_opcode_t;

/* Opcode metadata - used for optimization and verification */
typedef struct {
  fruity_opcode_t opcode;
  const char *name;

  /* Pebble effects */
  int creates_white;  /* Issues new white token? */
  int burns_white;    /* Burns white token? */
  int may_free;       /* May free memory? */
  int is_speculative; /* Creates Red shadow? */

  /* Stack effects */
  int stack_pop;  /* Number of values popped */
  int stack_push; /* Number of values pushed */

  /* Control flow */
  int is_branch;     /* Is a branch instruction? */
  int is_call;       /* Is a call instruction? */
  int is_return;     /* Is a return instruction? */
  int is_terminator; /* Terminates basic block? */

} fruity_opcode_metadata_t;

/* Opcode metadata table (defined in fruity_ir.c) */
extern const fruity_opcode_metadata_t fruity_opcode_table[];

/* Opcode query functions */
const char *fruity_opcode_name(fruity_opcode_t opcode);
int fruity_opcode_creates_white(fruity_opcode_t opcode);
int fruity_opcode_burns_white(fruity_opcode_t opcode);
int fruity_opcode_may_free(fruity_opcode_t opcode);
int fruity_opcode_is_terminator(fruity_opcode_t opcode);

#endif /* FRUITY_OPCODES_H */
