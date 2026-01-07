#!/usr/bin/env python3

# Complete list of missing opcodes for systematic implementation
import subprocess
import re

def get_all_missing_opcodes():
    try:
        with open('include/il_decoder.h', 'r') as f:
            header_content = f.read()
    except FileNotFoundError:
        print("Could not find il_decoder.h")
        return []

    # Extract defined opcodes
    defined_opcodes = set()
    for line in header_content.split('\n'):
        if 'CIL_OPCODE_' in line and '=' in line:
            match = re.search(r'CIL_OPCODE_(\w+)\s*=\s*0x([0-9a-fA-F]+)', line)
            if match:
                opcode_name = match.group(1)
                opcode_value = match.group(2)
                defined_opcodes.add(opcode_name)

    # Get implemented opcodes from execution engine
    try:
        with open('src/execution_engine.c', 'r') as f:
            exec_content = f.read()
    except FileNotFoundError:
        print("Could not find execution_engine.c")
        return []

    # Extract implemented opcodes  
    implemented_opcodes = set()
    for line in exec_content.split('\n'):
        if 'case CIL_OPCODE_' in line:
            match = re.search(r'CIL_OPCODE_(\w+)', line)
            if match:
                implemented_opcodes.add(match.group(1))

    # Find missing opcodes
    missing_opcodes = defined_opcodes - implemented_opcodes
    
    return sorted(missing_opcodes)

if __name__ == "__main__":
    missing = get_all_missing_opcodes()
    
    print(f"Total missing opcodes: {len(missing)}")
    print("\nComplete list of missing opcodes:")
    
    # Group opcodes by category for systematic implementation
    categories = {
        'Control Flow': [],
        'Method Invocation': [],
        'Object Model': [],
        'Memory Access': [],
        'Type System': [],
        'Exception Handling': [],
        'Arithmetic': [],
        'Other': []
    }
    
    for opcode in missing:
        if any(x in opcode for x in ['BR', 'JMP', 'SWITCH']):
            categories['Control Flow'].append(opcode)
        elif any(x in opcode for x in ['CALL', 'LDFTN', 'LDVIRTFTN', 'JMP']):
            categories['Method Invocation'].append(opcode)
        elif any(x in opcode for x in ['NEW', 'BOX', 'UNBOX', 'LDFLD', 'STFLD', 'CASTCLASS', 'ISINST', 'LDOBJ', 'STOBJ']):
            categories['Object Model'].append(opcode)
        elif any(x in opcode for x in ['LDELEM', 'STELEM', 'LDIND', 'STIND', 'CPBLK', 'INITBLK']):
            categories['Memory Access'].append(opcode)
        elif any(x in opcode for x in ['CONV', 'ARGLIST', 'SIZEOF', 'LDTOKEN']):
            categories['Type System'].append(opcode)
        elif any(x in opcode for x in ['THROW', 'ENDFILTER', 'RETHROW', 'FINALLY']):
            categories['Exception Handling'].append(opcode)
        elif any(x in opcode for x in ['OVF']):
            categories['Arithmetic'].append(opcode)
        else:
            categories['Other'].append(opcode)
    
    # Print by category
    for category, opcodes in categories.items():
        if opcodes:
            print(f"\n=== {category} ({len(opcodes)} opcodes) ===")
            for i, opcode in enumerate(opcodes):
                if i % 5 == 0 and i > 0:
                    print()
                print(f"{opcode:<20}", end="")
            print()  # newline
    
    print(f"\nTotal: {sum(len(v) for v in categories.values())} opcodes")