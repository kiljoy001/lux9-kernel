/* fruity_serialize.h - Fruity IR Binary Serialization
 *
 * Simple binary format for passing Fruity IR modules to the kernel.
 * Format designed for simplicity and kernel compatibility.
 */

#ifndef FRUITY_SERIALIZE_H
#define FRUITY_SERIALIZE_H

#include "fruity_ir.h"

/* Binary format header */
typedef struct {
	u32int magic;           /* 0x46525549 ("FRUI") */
	u32int version;         /* Format version = 1 */
	u32int function_count;  /* Number of functions */
	u32int reserved;        /* Padding */
} fruity_module_header_t;

typedef struct {
	u32int name_offset;     /* Offset to name string */
	u32int name_len;        /* Length of name */
	u32int method_token;    /* ECMA-335 metadata token */
	u32int block_count;     /* Number of basic blocks */
	u32int arg_count;       /* Number of arguments */
	u32int local_count;     /* Number of locals */
	u32int return_type;     /* CLR type token */
	u32int max_stack_depth; /* Maximum stack depth */
} fruity_function_header_t;

typedef struct {
	u32int block_id;        /* Basic block ID */
	u32int instruction_count; /* Number of instructions */
	u32int successor_count; /* Number of successors */
	u32int reserved;        /* Padding */
} fruity_block_header_t;

typedef struct {
	u32int opcode;          /* Fruity opcode */
	u32int operand_type;    /* Operand type */
	union {
		s32int i32;     /* Immediate int32 */
		s64int i64;     /* Immediate int64 */
		u32int index;   /* Local/arg/field index */
		u32int token;   /* Type/method token */
		u32int block_id; /* Branch target block ID */
	} operand;
} fruity_instruction_serial_t;

/* Serialization functions */
int fruity_module_serialize(fruity_module_t *module, 
                             void *buffer, 
                             ulong buffer_size,
                             ulong *bytes_written);

fruity_module_t* fruity_module_deserialize(void *buffer, 
                                            ulong buffer_size,
                                            char *errorbuf,
                                            ulong errorbuf_size);

#endif /* FRUITY_SERIALIZE_H */
