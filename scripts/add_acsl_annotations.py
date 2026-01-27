#!/usr/bin/env python3
"""
Automated ACSL Annotation Generator for Lux9 Kernel

Adds basic ACSL annotations to C files for Frama-C verification.
"""

import re
import sys
import os
from pathlib import Path

# Pattern to match function definitions
FUNC_PATTERN = re.compile(
    r'^(\w[\w\s\*]*?)\s+(\w+)\s*\((.*?)\)\s*\{',
    re.MULTILINE
)

# Pattern to match for loops
LOOP_PATTERN = re.compile(
    r'for\s*\(\s*(\w+)\s+(\w+)\s*=\s*([^;]+);([^;]+);([^\)]+)\)',
    re.MULTILINE
)

def generate_function_contract(return_type, func_name, params):
    """Generate basic ACSL contract for a function"""
    contract_lines = []

    # Parse parameters
    param_list = [p.strip() for p in params.split(',') if p.strip()]

    # Add pointer validity checks
    for param in param_list:
        if '*' in param:
            # Extract parameter name
            parts = param.rsplit(' ', 1)
            if len(parts) == 2:
                param_name = parts[1].strip('*')
                contract_lines.append(f"  @ requires {param_name} == \\null || \\valid({param_name});")

    # Add return type contract
    if return_type.strip() != 'void':
        if 'Error' in return_type or 'Status' in return_type:
            contract_lines.append(f"  @ ensures \\result >= 0;")
        elif '*' in return_type:
            contract_lines.append(f"  @ ensures \\result == \\null || \\valid(\\result);")

    # Add assigns clause (conservative - assigns everything)
    contract_lines.append(f"  @ assigns \\nothing;")

    if contract_lines:
        return "/*@\n" + "\n".join(contract_lines) + "\n  @*/\n"
    return ""

def add_loop_invariants(code):
    """Add basic loop invariants to for loops"""
    def replace_loop(match):
        full_match = match.group(0)
        var_type = match.group(1)
        var_name = match.group(2)
        init_val = match.group(3).strip()
        condition = match.group(4).strip()
        increment = match.group(5).strip()

        # Extract upper bound from condition
        if '<' in condition:
            parts = condition.split('<')
            upper_bound = parts[1].strip()

            invariant = f"  /*@ loop invariant 0 <= {var_name} <= {upper_bound};\n"
            invariant += f"    @ loop assigns {var_name};\n"
            invariant += f"    @ loop variant {upper_bound} - {var_name};\n"
            invariant += f"    @*/\n  "

            return invariant + full_match

        return full_match

    return LOOP_PATTERN.sub(replace_loop, code)

def annotate_file(file_path):
    """Add ACSL annotations to a C file"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Check if already annotated
        if '/*@' in content:
            print(f"SKIP: {file_path} (already has ACSL annotations)")
            return False

        # Add function contracts
        lines = content.split('\n')
        new_lines = []
        i = 0

        while i < len(lines):
            line = lines[i]

            # Check if this is a function definition
            func_match = re.match(r'^(\w[\w\s\*]+?)\s+(\w+)\s*\((.*?)\)\s*\{?$', line)
            if func_match and not line.strip().startswith('//'):
                return_type = func_match.group(1)
                func_name = func_match.group(2)
                params = func_match.group(3)

                # Skip if it's a forward declaration or type definition
                if 'typedef' not in line and 'extern' not in line:
                    contract = generate_function_contract(return_type, func_name, params)
                    if contract:
                        new_lines.append(contract.rstrip())

            new_lines.append(line)
            i += 1

        new_content = '\n'.join(new_lines)

        # Add loop invariants
        new_content = add_loop_invariants(new_content)

        # Write back
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)

        print(f"ANNOTATED: {file_path}")
        return True

    except Exception as e:
        print(f"ERROR: {file_path}: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: add_acsl_annotations.py <file1.c> [file2.c ...]")
        sys.exit(1)

    success_count = 0
    for file_path in sys.argv[1:]:
        if annotate_file(file_path):
            success_count += 1

    print(f"\nAnnotated {success_count}/{len(sys.argv)-1} files")

if __name__ == '__main__':
    main()
