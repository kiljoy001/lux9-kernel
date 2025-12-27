#!/usr/bin/env python3
"""
Extract CIL opcodes from ECMA-335 specification
Creates structured data for formal verification
"""

import re
import json
import sys

# CIL Opcode definitions from ECMA-335 Partition III
# This is the authoritative list from the specification
CIL_OPCODES = [
    # Base instructions
    (0x00, "nop", "No operation", "... -> ..."),
    (0x01, "break", "Breakpoint", "... -> ..."),
    
    # Load argument
    (0x02, "ldarg.0", "Load argument 0", "... -> ..., value"),
    (0x03, "ldarg.1", "Load argument 1", "... -> ..., value"),
    (0x04, "ldarg.2", "Load argument 2", "... -> ..., value"),
    (0x05, "ldarg.3", "Load argument 3", "... -> ..., value"),
    
    # Load local
    (0x06, "ldloc.0", "Load local 0", "... -> ..., value"),
    (0x07, "ldloc.1", "Load local 1", "... -> ..., value"),
    (0x08, "ldloc.2", "Load local 2", "... -> ..., value"),
    (0x09, "ldloc.3", "Load local 3", "... -> ..., value"),
    
    # Store local
    (0x0A, "stloc.0", "Store to local 0", "..., value -> ..."),
    (0x0B, "stloc.1", "Store to local 1", "..., value -> ..."),
    (0x0C, "stloc.2", "Store to local 2", "..., value -> ..."),
    (0x0D, "stloc.3", "Store to local 3", "..., value -> ..."),
    
    # Short form loads/stores
    (0x0E, "ldarg.s", "Load argument (short)", "... -> ..., value"),
    (0x0F, "ldarga.s", "Load argument address (short)", "... -> ..., address"),
    (0x10, "starg.s", "Store argument (short)", "..., value -> ..."),
    (0x11, "ldloc.s", "Load local (short)", "... -> ..., value"),
    (0x12, "ldloca.s", "Load local address (short)", "... -> ..., address"),
    (0x13, "stloc.s", "Store local (short)", "..., value -> ..."),
    
    # Constants
    (0x14, "ldnull", "Load null", "... -> ..., null"),
    (0x15, "ldc.i4.m1", "Load -1", "... -> ..., -1"),
    (0x16, "ldc.i4.0", "Load 0", "... -> ..., 0"),
    (0x17, "ldc.i4.1", "Load 1", "... -> ..., 1"),
    (0x18, "ldc.i4.2", "Load 2", "... -> ..., 2"),
    (0x19, "ldc.i4.3", "Load 3", "... -> ..., 3"),
    (0x1A, "ldc.i4.4", "Load 4", "... -> ..., 4"),
    (0x1B, "ldc.i4.5", "Load 5", "... -> ..., 5"),
    (0x1C, "ldc.i4.6", "Load 6", "... -> ..., 6"),
    (0x1D, "ldc.i4.7", "Load 7", "... -> ..., 7"),
    (0x1E, "ldc.i4.8", "Load 8", "... -> ..., 8"),
    (0x1F, "ldc.i4.s", "Load int8 as int32", "... -> ..., value"),
    (0x20, "ldc.i4", "Load int32", "... -> ..., value"),
    (0x21, "ldc.i8", "Load int64", "... -> ..., value"),
    (0x22, "ldc.r4", "Load float32", "... -> ..., value"),
    (0x23, "ldc.r8", "Load float64", "... -> ..., value"),
    
    # Stack manipulation
    (0x25, "dup", "Duplicate", "..., value -> ..., value, value"),
    (0x26, "pop", "Pop", "..., value -> ..."),
    
    # Method calls
    (0x27, "jmp", "Jump to method", "... -> ..."),
    (0x28, "call", "Call method", "..., arg1, ..., argN -> ..., retVal"),
    (0x29, "calli", "Call indirect", "..., arg1, ..., argN, fptr -> ..., retVal"),
    (0x2A, "ret", "Return", "..., retVal -> ..."),
    
    # Branches (short form)
    (0x2B, "br.s", "Branch (short)", "... -> ..."),
    (0x2C, "brfalse.s", "Branch if false (short)", "..., value -> ..."),
    (0x2D, "brtrue.s", "Branch if true (short)", "..., value -> ..."),
    (0x2E, "beq.s", "Branch if equal (short)", "..., value1, value2 -> ..."),
    (0x2F, "bge.s", "Branch if >= (short)", "..., value1, value2 -> ..."),
    (0x30, "bgt.s", "Branch if > (short)", "..., value1, value2 -> ..."),
    (0x31, "ble.s", "Branch if <= (short)", "..., value1, value2 -> ..."),
    (0x32, "blt.s", "Branch if < (short)", "..., value1, value2 -> ..."),
    (0x33, "bne.un.s", "Branch if != (short)", "..., value1, value2 -> ..."),
    (0x34, "bge.un.s", "Branch if >= unsigned (short)", "..., value1, value2 -> ..."),
    (0x35, "bgt.un.s", "Branch if > unsigned (short)", "..., value1, value2 -> ..."),
    (0x36, "ble.un.s", "Branch if <= unsigned (short)", "..., value1, value2 -> ..."),
    (0x37, "blt.un.s", "Branch if < unsigned (short)", "..., value1, value2 -> ..."),
    
    # Branches (long form)
    (0x38, "br", "Branch", "... -> ..."),
    (0x39, "brfalse", "Branch if false", "..., value -> ..."),
    (0x3A, "brtrue", "Branch if true", "..., value -> ..."),
    (0x3B, "beq", "Branch if equal", "..., value1, value2 -> ..."),
    (0x3C, "bge", "Branch if >=", "..., value1, value2 -> ..."),
    (0x3D, "bgt", "Branch if >", "..., value1, value2 -> ..."),
    (0x3E, "ble", "Branch if <=", "..., value1, value2 -> ..."),
    (0x3F, "blt", "Branch if <", "..., value1, value2 -> ..."),
    (0x40, "bne.un", "Branch if !=", "..., value1, value2 -> ..."),
    (0x41, "bge.un", "Branch if >= unsigned", "..., value1, value2 -> ..."),
    (0x42, "bgt.un", "Branch if > unsigned", "..., value1, value2 -> ..."),
    (0x43, "ble.un", "Branch if <= unsigned", "..., value1, value2 -> ..."),
    (0x44, "blt.un", "Branch if < unsigned", "..., value1, value2 -> ..."),
    
    # Switch
    (0x45, "switch", "Switch", "..., value -> ..."),
    
    # Indirect loads
    (0x46, "ldind.i1", "Load int8 indirect", "..., addr -> ..., value"),
    (0x47, "ldind.u1", "Load uint8 indirect", "..., addr -> ..., value"),
    (0x48, "ldind.i2", "Load int16 indirect", "..., addr -> ..., value"),
    (0x49, "ldind.u2", "Load uint16 indirect", "..., addr -> ..., value"),
    (0x4A, "ldind.i4", "Load int32 indirect", "..., addr -> ..., value"),
    (0x4B, "ldind.u4", "Load uint32 indirect", "..., addr -> ..., value"),
    (0x4C, "ldind.i8", "Load int64 indirect", "..., addr -> ..., value"),
    (0x4D, "ldind.i", "Load native int indirect", "..., addr -> ..., value"),
    (0x4E, "ldind.r4", "Load float32 indirect", "..., addr -> ..., value"),
    (0x4F, "ldind.r8", "Load float64 indirect", "..., addr -> ..., value"),
    (0x50, "ldind.ref", "Load reference indirect", "..., addr -> ..., value"),
    
    # Indirect stores
    (0x51, "stind.ref", "Store reference indirect", "..., addr, value -> ..."),
    (0x52, "stind.i1", "Store int8 indirect", "..., addr, value -> ..."),
    (0x53, "stind.i2", "Store int16 indirect", "..., addr, value -> ..."),
    (0x54, "stind.i4", "Store int32 indirect", "..., addr, value -> ..."),
    (0x55, "stind.i8", "Store int64 indirect", "..., addr, value -> ..."),
    (0x56, "stind.r4", "Store float32 indirect", "..., addr, value -> ..."),
    (0x57, "stind.r8", "Store float64 indirect", "..., addr, value -> ..."),
    
    # Arithmetic
    (0x58, "add", "Add", "..., value1, value2 -> ..., result"),
    (0x59, "sub", "Subtract", "..., value1, value2 -> ..., result"),
    (0x5A, "mul", "Multiply", "..., value1, value2 -> ..., result"),
    (0x5B, "div", "Divide", "..., value1, value2 -> ..., result"),
    (0x5C, "div.un", "Divide unsigned", "..., value1, value2 -> ..., result"),
    (0x5D, "rem", "Remainder", "..., value1, value2 -> ..., result"),
    (0x5E, "rem.un", "Remainder unsigned", "..., value1, value2 -> ..., result"),
    (0x5F, "and", "Bitwise AND", "..., value1, value2 -> ..., result"),
    (0x60, "or", "Bitwise OR", "..., value1, value2 -> ..., result"),
    (0x61, "xor", "Bitwise XOR", "..., value1, value2 -> ..., result"),
    (0x62, "shl", "Shift left", "..., value, shift -> ..., result"),
    (0x63, "shr", "Shift right", "..., value, shift -> ..., result"),
    (0x64, "shr.un", "Shift right unsigned", "..., value, shift -> ..., result"),
    (0x65, "neg", "Negate", "..., value -> ..., result"),
    (0x66, "not", "Bitwise NOT", "..., value -> ..., result"),
    
    # Conversions
    (0x67, "conv.i1", "Convert to int8", "..., value -> ..., result"),
    (0x68, "conv.i2", "Convert to int16", "..., value -> ..., result"),
    (0x69, "conv.i4", "Convert to int32", "..., value -> ..., result"),
    (0x6A, "conv.i8", "Convert to int64", "..., value -> ..., result"),
    (0x6B, "conv.r4", "Convert to float32", "..., value -> ..., result"),
    (0x6C, "conv.r8", "Convert to float64", "..., value -> ..., result"),
    (0x6D, "conv.u4", "Convert to uint32", "..., value -> ..., result"),
    (0x6E, "conv.u8", "Convert to uint64", "..., value -> ..., result"),
    
    # Virtual calls
    (0x6F, "callvirt", "Call virtual", "..., obj, arg1, ..., argN -> ..., retVal"),
    
    # Object/string
    (0x70, "ldstr", "Load string", "... -> ..., string"),
    
    # Object creation
    (0x73, "newobj", "New object", "..., arg1, ..., argN -> ..., obj"),
    (0x74, "castclass", "Cast class", "..., obj -> ..., obj"),
    (0x75, "isinst", "Is instance", "..., obj -> ..., result"),
    
    # Conversions with overflow check
    (0x76, "conv.r.un", "Convert unsigned to float", "..., value -> ..., result"),
    
    # Unbox
    (0x79, "unbox", "Unbox value type", "..., obj -> ..., valuePtr"),
    
    # Exceptions
    (0x7A, "throw", "Throw exception", "..., obj -> ..."),
    
    # Fields
    (0x7B, "ldfld", "Load field", "..., obj -> ..., value"),
    (0x7C, "ldflda", "Load field address", "..., obj -> ..., address"),
    (0x7D, "stfld", "Store field", "..., obj, value -> ..."),
    (0x7E, "ldsfld", "Load static field", "... -> ..., value"),
    (0x7F, "ldsflda", "Load static field address", "... -> ..., address"),
    (0x80, "stsfld", "Store static field", "..., value -> ..."),
    
    # Object storage
    (0x81, "stobj", "Store object", "..., addr, value -> ..."),
    
    # Conversions with overflow
    (0x82, "conv.ovf.i1.un", "Convert to int8 with overflow check", "..., value -> ..., result"),
    (0x83, "conv.ovf.i2.un", "Convert to int16 with overflow check", "..., value -> ..., result"),
    (0x84, "conv.ovf.i4.un", "Convert to int32 with overflow check", "..., value -> ..., result"),
    (0x85, "conv.ovf.i8.un", "Convert to int64 with overflow check", "..., value -> ..., result"),
    (0x86, "conv.ovf.u1.un", "Convert to uint8 with overflow check", "..., value -> ..., result"),
    (0x87, "conv.ovf.u2.un", "Convert to uint16 with overflow check", "..., value -> ..., result"),
    (0x88, "conv.ovf.u4.un", "Convert to uint32 with overflow check", "..., value -> ..., result"),
    (0x89, "conv.ovf.u8.un", "Convert to uint64 with overflow check", "..., value -> ..., result"),
    (0x8A, "conv.ovf.i.un", "Convert to native int with overflow check", "..., value -> ..., result"),
    (0x8B, "conv.ovf.u.un", "Convert to native uint with overflow check", "..., value -> ..., result"),
    
    # Boxing
    (0x8C, "box", "Box value type", "..., value -> ..., obj"),
    
    # Arrays
    (0x8D, "newarr", "New array", "..., length -> ..., array"),
    (0x8E, "ldlen", "Load array length", "..., array -> ..., length"),
    (0x8F, "ldelema", "Load element address", "..., array, index -> ..., address"),
    
    # Array element access
    (0x90, "ldelem.i1", "Load int8 element", "..., array, index -> ..., value"),
    (0x91, "ldelem.u1", "Load uint8 element", "..., array, index -> ..., value"),
    (0x92, "ldelem.i2", "Load int16 element", "..., array, index -> ..., value"),
    (0x93, "ldelem.u2", "Load uint16 element", "..., array, index -> ..., value"),
    (0x94, "ldelem.i4", "Load int32 element", "..., array, index -> ..., value"),
    (0x95, "ldelem.u4", "Load uint32 element", "..., array, index -> ..., value"),
    (0x96, "ldelem.i8", "Load int64 element", "..., array, index -> ..., value"),
    (0x97, "ldelem.i", "Load native int element", "..., array, index -> ..., value"),
    (0x98, "ldelem.r4", "Load float32 element", "..., array, index -> ..., value"),
    (0x99, "ldelem.r8", "Load float64 element", "..., array, index -> ..., value"),
    (0x9A, "ldelem.ref", "Load reference element", "..., array, index -> ..., value"),
    
    # Store array elements
    (0x9B, "stelem.i", "Store native int element", "..., array, index, value -> ..."),
    (0x9C, "stelem.i1", "Store int8 element", "..., array, index, value -> ..."),
    (0x9D, "stelem.i2", "Store int16 element", "..., array, index, value -> ..."),
    (0x9E, "stelem.i4", "Store int32 element", "..., array, index, value -> ..."),
    (0x9F, "stelem.i8", "Store int64 element", "..., array, index, value -> ..."),
    (0xA0, "stelem.r4", "Store float32 element", "..., array, index, value -> ..."),
    (0xA1, "stelem.r8", "Store float64 element", "..., array, index, value -> ..."),
    (0xA2, "stelem.ref", "Store reference element", "..., array, index, value -> ..."),
    (0xA3, "ldelem", "Load element", "..., array, index -> ..., value"),
    (0xA4, "stelem", "Store element", "..., array, index, value -> ..."),
    (0xA5, "unbox.any", "Unbox any", "..., obj -> ..., value"),
    
    # Conversions with overflow
    (0xB3, "conv.ovf.i1", "Convert to int8 with overflow", "..., value -> ..., result"),
    (0xB4, "conv.ovf.u1", "Convert to uint8 with overflow", "..., value -> ..., result"),
    (0xB5, "conv.ovf.i2", "Convert to int16 with overflow", "..., value -> ..., result"),
    (0xB6, "conv.ovf.u2", "Convert to uint16 with overflow", "..., value -> ..., result"),
    (0xB7, "conv.ovf.i4", "Convert to int32 with overflow", "..., value -> ..., result"),
    (0xB8, "conv.ovf.u4", "Convert to uint32 with overflow", "..., value -> ..., result"),
    (0xB9, "conv.ovf.i8", "Convert to int64 with overflow", "..., value -> ..., result"),
    (0xBA, "conv.ovf.u8", "Convert to uint64 with overflow", "..., value -> ..., result"),
    
    # References
    (0xC2, "refanyval", "Load value from typed reference", "..., typedRef -> ..., address"),
    (0xC3, "ckfinite", "Check finite", "..., value -> ..., value"),
    
    # Types
    (0xC6, "mkrefany", "Make typed reference", "..., ptr -> ..., typedRef"),
    
    # Tokens
    (0xD0, "ldtoken", "Load token", "... -> ..., token"),
    
    # Conversions
    (0xD1, "conv.u2", "Convert to uint16", "..., value -> ..., result"),
    (0xD2, "conv.u1", "Convert to uint8", "..., value -> ..., result"),
    (0xD3, "conv.i", "Convert to native int", "..., value -> ..., result"),
    (0xD4, "conv.ovf.i", "Convert to native int with overflow", "..., value -> ..., result"),
    (0xD5, "conv.ovf.u", "Convert to native uint with overflow", "..., value -> ..., result"),
    
    # Arithmetic with overflow
    (0xD6, "add.ovf", "Add with overflow check", "..., value1, value2 -> ..., result"),
    (0xD7, "add.ovf.un", "Add unsigned with overflow", "..., value1, value2 -> ..., result"),
    (0xD8, "mul.ovf", "Multiply with overflow", "..., value1, value2 -> ..., result"),
    (0xD9, "mul.ovf.un", "Multiply unsigned with overflow", "..., value1, value2 -> ..., result"),
    (0xDA, "sub.ovf", "Subtract with overflow", "..., value1, value2 -> ..., result"),
    (0xDB, "sub.ovf.un", "Subtract unsigned with overflow", "..., value1, value2 -> ..., result"),
    
    # Leave
    (0xDC, "endfinally", "End finally/fault", "... -> ..."),
    (0xDD, "leave", "Leave protected region", "... -> ..."),
    (0xDE, "leave.s", "Leave protected region (short)", "... -> ..."),
    
    # Native int
    (0xDF, "stind.i", "Store native int indirect", "..., addr, value -> ..."),
    (0xE0, "conv.u", "Convert to native uint", "..., value -> ..., result"),
]

# Two-byte opcodes (0xFE prefix)
TWO_BYTE_OPCODES = [
    (0xFE00, "arglist", "Get argument list", "... -> ..., argListHandle"),
    (0xFE01, "ceq", "Compare equal", "..., value1, value2 -> ..., result"),
    (0xFE02, "cgt", "Compare greater than", "..., value1, value2 -> ..., result"),
    (0xFE03, "cgt.un", "Compare greater unsigned", "..., value1, value2 -> ..., result"),
    (0xFE04, "clt", "Compare less than", "..., value1, value2 -> ..., result"),
    (0xFE05, "clt.un", "Compare less unsigned", "..., value1, value2 -> ..., result"),
    (0xFE06, "ldftn", "Load function pointer", "... -> ..., fptr"),
    (0xFE07, "ldvirtftn", "Load virtual function pointer", "..., obj -> ..., fptr"),
    (0xFE09, "ldarg", "Load argument", "... -> ..., value"),
    (0xFE0A, "ldarga", "Load argument address", "... -> ..., address"),
    (0xFE0B, "starg", "Store argument", "..., value -> ..."),
    (0xFE0C, "ldloc", "Load local", "... -> ..., value"),
    (0xFE0D, "ldloca", "Load local address", "... -> ..., address"),
    (0xFE0E, "stloc", "Store local", "..., value -> ..."),
    (0xFE0F, "localloc", "Allocate local memory", "..., size -> ..., address"),
    (0xFE11, "endfilter", "End filter", "..., value -> ..."),
    (0xFE12, "unaligned.", "Unaligned prefix", "... -> ..."),
    (0xFE13, "volatile.", "Volatile prefix", "... -> ..."),
    (0xFE14, "tail.", "Tail call prefix", "... -> ..."),
    (0xFE15, "initobj", "Initialize object", "..., address -> ..."),
    (0xFE16, "constrained.", "Constrained prefix", "... -> ..."),
    (0xFE17, "cpblk", "Copy block", "..., dest, src, size -> ..."),
    (0xFE18, "initblk", "Initialize block", "..., addr, value, size -> ..."),
    (0xFE19, "no.", "No typecheck prefix", "... -> ..."),
    (0xFE1A, "rethrow", "Rethrow exception", "... -> ..."),
    (0xFE1C, "sizeof", "Size of type", "... -> ..., size"),
    (0xFE1D, "refanytype", "Get type from typed reference", "..., typedRef -> ..., type"),
    (0xFE1E, "readonly.", "Readonly prefix", "... -> ..."),
]

def generate_coq_definitions():
    """Generate Coq definitions for CIL opcodes"""
    coq = """(* Auto-generated CIL Opcode definitions from ECMA-335 *)
(* DO NOT EDIT - Generated from extract_opcodes.py *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Import ListNotations.

(* CIL Opcode type *)
Inductive CILOpcode : Type :=
"""
    
    # Generate opcode constructors
    for opcode, name, desc, stack in CIL_OPCODES:
        constructor = name.replace('.', '_').replace('-', '_').upper()
        coq += f"  | {constructor}\n"
    
    for opcode, name, desc, stack in TWO_BYTE_OPCODES:
        constructor = name.replace('.', '_').replace('-', '_').upper()
        coq += f"  | {constructor}\n"
    
    coq += ".\n\n"
    
    # Generate opcode to byte mapping
    coq += "(* Opcode to byte encoding *)\n"
    coq += "Definition opcode_to_byte (op : CILOpcode) : Z :=\n"
    coq += "  match op with\n"
    
    for opcode, name, desc, stack in CIL_OPCODES:
        constructor = name.replace('.', '_').replace('-', '_').upper()
        coq += f"  | {constructor} => {hex(opcode)}%Z\n"
    
    for opcode, name, desc, stack in TWO_BYTE_OPCODES:
        constructor = name.replace('.', '_').replace('-', '_').upper()
        coq += f"  | {constructor} => {hex(opcode)}%Z\n"
    
    coq += "  end.\n\n"
    
    # Generate stack transition function
    coq += "(* Stack transition types *)\n"
    coq += "Inductive StackTransition : Type :=\n"
    coq += "  | PopPush : nat -> nat -> StackTransition\n"
    coq += "  | Variable : StackTransition.\n\n"
    
    coq += "(* Get stack transition for opcode *)\n"
    coq += "Definition opcode_stack_transition (op : CILOpcode) : StackTransition :=\n"
    coq += "  match op with\n"
    
    for opcode, name, desc, stack in CIL_OPCODES:
        constructor = name.replace('.', '_').replace('-', '_').upper()
        # Parse stack transition
        if "..." in stack:
            parts = stack.split("->")
            before = parts[0].strip()
            after = parts[1].strip() if len(parts) > 1 else "..."
            
            # Count pops and pushes
            pops = before.count(",") + (1 if "value" in before or "obj" in before else 0)
            pushes = after.count(",") + (1 if "value" in after or "result" in after else 0)
            
            coq += f"  | {constructor} => PopPush {pops} {pushes}\n"
        else:
            coq += f"  | {constructor} => Variable\n"
    
    coq += "  end.\n"
    
    return coq

def generate_c_implementation():
    """Generate C implementation skeleton"""
    c_code = """/* Auto-generated CIL Opcode implementation from ECMA-335 */
/* DO NOT EDIT - Generated from extract_opcodes.py */

#include <stdint.h>
#include <stdbool.h>
#include "clr_types.h"

/* Execute single CIL opcode */
clr_result_t execute_opcode(clr_state_t *state, uint8_t opcode) {
    switch (opcode) {
"""
    
    for opcode, name, desc, stack in CIL_OPCODES:
        c_code += f"""    case 0x{opcode:02X}: /* {name} - {desc} */
        /* Stack: {stack} */
        return execute_{name.replace('.', '_').replace('-', '_')}(state);
        
"""
    
    c_code += """    default:
        return CLR_ERROR_INVALID_OPCODE;
    }
}

/* Two-byte opcodes */
clr_result_t execute_two_byte_opcode(clr_state_t *state, uint8_t opcode2) {
    switch (opcode2) {
"""
    
    for opcode, name, desc, stack in TWO_BYTE_OPCODES:
        byte2 = opcode & 0xFF
        c_code += f"""    case 0x{byte2:02X}: /* {name} - {desc} */
        /* Stack: {stack} */
        return execute_{name.replace('.', '_').replace('-', '_')}(state);
        
"""
    
    c_code += """    default:
        return CLR_ERROR_INVALID_OPCODE;
    }
}
"""
    
    return c_code

def generate_solr_schema():
    """Generate Solr schema for opcode indexing"""
    schema = {
        "opcodes": []
    }
    
    for opcode, name, desc, stack in CIL_OPCODES + TWO_BYTE_OPCODES:
        schema["opcodes"].append({
            "id": f"opcode_{opcode:04X}",
            "hex": f"0x{opcode:04X}",
            "name": name,
            "description": desc,
            "stack_transition": stack,
            "category": categorize_opcode(name),
            "spec_section": "ECMA-335 Partition III",
            "formal_verified": False,
            "implementation_status": "pending"
        })
    
    return json.dumps(schema, indent=2)

def categorize_opcode(name):
    """Categorize opcode by type"""
    if name.startswith("ld") or name.startswith("st"):
        return "load_store"
    elif name.startswith("br") or name == "switch" or name == "ret":
        return "control_flow"
    elif name.startswith("add") or name.startswith("sub") or name.startswith("mul") or name.startswith("div"):
        return "arithmetic"
    elif name.startswith("conv"):
        return "conversion"
    elif name.startswith("call") or name == "jmp":
        return "method_call"
    elif "elem" in name or "arr" in name:
        return "array"
    elif "fld" in name:
        return "field"
    elif name in ["newobj", "castclass", "isinst", "box", "unbox"]:
        return "object"
    elif name in ["throw", "rethrow", "endfinally", "endfilter"]:
        return "exception"
    elif name in ["ceq", "cgt", "clt"]:
        return "comparison"
    else:
        return "misc"

if __name__ == "__main__":
    print("Generating formal specifications for CIL opcodes...")
    
    # Generate Coq definitions
    with open("cil_opcodes.v", "w") as f:
        f.write(generate_coq_definitions())
    print("✓ Generated cil_opcodes.v")
    
    # Generate C implementation
    with open("cil_opcodes.c", "w") as f:
        f.write(generate_c_implementation())
    print("✓ Generated cil_opcodes.c")
    
    # Generate Solr schema
    with open("cil_opcodes_solr.json", "w") as f:
        f.write(generate_solr_schema())
    print("✓ Generated cil_opcodes_solr.json")
    
    print(f"\nTotal opcodes: {len(CIL_OPCODES) + len(TWO_BYTE_OPCODES)}")
    print(f"Single-byte: {len(CIL_OPCODES)}")
    print(f"Two-byte: {len(TWO_BYTE_OPCODES)}")