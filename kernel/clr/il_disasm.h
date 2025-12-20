/* il_disasm.h - IL Bytecode Disassembler
 *
 * Disassembles CIL bytecode to human-readable format.
 * Useful for debugging and understanding IL structure.
 */

#ifndef IL_DISASM_H
#define IL_DISASM_H

#include "il_parser.h"

/* Disassemble method IL bytecode to stdout */
void il_disassemble_method(il_method_t *method);

/* Disassemble single IL instruction */
/* Returns: number of bytes consumed */
int il_disassemble_instruction(const uint8_t *il, size_t offset, size_t max_offset);

/* Get IL opcode name */
const char* il_opcode_name(uint8_t opcode);

#endif /* IL_DISASM_H */
