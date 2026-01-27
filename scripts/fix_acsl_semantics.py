#!/usr/bin/env python3
"""
Fix common semantic errors in ACSL annotations

Fixes:
1. Remove `assigns \nothing` from functions that modify parameters
2. Fix pointer validity checks (remove \null || pattern)
3. Add proper assigns clauses based on what the function actually modifies
"""

import re
import sys

def find_modified_variables(func_body):
    """Identify variables/fields that are modified in function body"""
    modified = set()

    # Pattern: var->field = ...
    for match in re.finditer(r'(\w+)->(\w+)\s*=', func_body):
        param = match.group(1)
        field = match.group(2)
        modified.add(f"{param}->{field}")

    # Pattern: *var = ...
    for match in re.finditer(r'\*(\w+)\s*=', func_body):
        param = match.group(1)
        modified.add(f"*{param}")

    # Pattern: var[index] = ...
    for match in re.finditer(r'(\w+)\[', func_body):
        param = match.group(1)
        # Check if this is being assigned to
        if '=' in func_body[match.end():match.end()+50]:
            modified.add(f"{param}[0..]")

    return modified

def has_external_calls(func_body):
    """Check if function calls external functions (likely has side effects)"""
    # Common external function patterns
    external_funcs = ['print', 'xalloc', 'malloc', 'free', 'memcpy', 'memset',
                     'qlock', 'qunlock', 'lock', 'unlock', 'error',
                     'crypto_', 'ledger_', 'random', 'nsec']

    for func in external_funcs:
        if func in func_body:
            return True
    return False

def fix_function_annotation(match):
    """Fix a single function's ACSL annotation"""
    annotation = match.group(1)
    func_signature = match.group(2)
    func_body_start = match.group(3)

    # Extract function body (simplified - just look ahead a bit)
    # In real implementation, would need proper brace matching
    func_body_sample = func_body_start[:500] if func_body_start else ""

    # Check if annotation has incorrect `assigns \nothing`
    if 'assigns \\nothing' in annotation:
        modified = find_modified_variables(func_body_sample)
        has_calls = has_external_calls(func_body_sample)

        if modified or has_calls:
            # Replace assigns \nothing with something more appropriate
            if modified:
                assigns_list = ', '.join(sorted(modified))
                new_annotation = annotation.replace('assigns \\nothing;', f'assigns {assigns_list};')
            else:
                # Has external calls, just remove the assigns clause
                new_annotation = re.sub(r'\s*@\s*assigns\s+\\nothing;\s*', '', annotation)

            return new_annotation + func_signature + func_body_start

    # Fix == \null || \valid() pattern
    if '== \\null || \\valid' in annotation:
        # Replace with just \valid() - null check should be in requires
        new_annotation = re.sub(
            r'(\w+)\s*==\s*\\null\s*\|\|\s*\\valid\((\w+)\)',
            r'\\valid(\2)',
            annotation
        )
        return new_annotation + func_signature + func_body_start

    return match.group(0)

def fix_file(filepath):
    """Fix ACSL annotations in a file"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Pattern to match ACSL annotation + function
        pattern = r'(/\*@.*?@\*/)\s*([\w\s\*]+\s+\w+\s*\([^)]*\)\s*\{)(.*?)(?=\n\w|\n/\*|$)'

        new_content = re.sub(pattern, fix_function_annotation, content, flags=re.DOTALL)

        if new_content != content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"FIXED: {filepath}")
            return True
        else:
            print(f"NO CHANGES: {filepath}")
            return False

    except Exception as e:
        print(f"ERROR: {filepath}: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: fix_acsl_semantics.py <file1.c> [file2.c ...]")
        sys.exit(1)

    fixed_count = 0
    for filepath in sys.argv[1:]:
        if fix_file(filepath):
            fixed_count += 1

    print(f"\nFixed {fixed_count}/{len(sys.argv)-1} files")

if __name__ == '__main__':
    main()
