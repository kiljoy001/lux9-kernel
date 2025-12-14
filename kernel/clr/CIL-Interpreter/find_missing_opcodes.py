#!/usr/bin/env python3

# Script to find missing CIL opcodes using TDD approach
import subprocess
import re

# Get all defined opcodes from header
try:
    with open('include/il_decoder.h', 'r') as f:
        header_content = f.read()
except FileNotFoundError:
    print("Could not find il_decoder.h")
    exit(1)

# Extract defined opcodes
defined_opcodes = set()
for line in header_content.split('\n'):
    if 'CIL_OPCODE_' in line and '=' in line:
        match = re.search(r'CIL_OPCODE_(\w+)\s*=', line)
        if match:
            defined_opcodes.add(match.group(1))

# Get implemented opcodes from execution engine
try:
    with open('src/execution_engine.c', 'r') as f:
        exec_content = f.read()
except FileNotFoundError:
    print("Could not find execution_engine.c")
    exit(1)

# Extract implemented opcodes  
implemented_opcodes = set()
for line in exec_content.split('\n'):
    if 'case CIL_OPCODE_' in line:
        match = re.search(r'CIL_OPCODE_(\w+)', line)
        if match:
            implemented_opcodes.add(match.group(1))

# Find missing opcodes
missing_opcodes = defined_opcodes - implemented_opcodes

print(f"Total defined opcodes: {len(defined_opcodes)}")
print(f"Implemented opcodes: {len(implemented_opcodes)}")
print(f"Missing opcodes: {len(missing_opcodes)}")
print(f"\nFirst 10 missing opcodes:")
for i, opcode in enumerate(sorted(missing_opcodes):
    if i < 10:
        print(f"  {opcode}")
    else:
        break
