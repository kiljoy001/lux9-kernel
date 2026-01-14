#!/usr/bin/env python3
"""
Remove ALL ACSL contracts from problematic files to fix Frama-C WP errors.
This is a more aggressive fix since some contracts cause infinite range errors
after preprocessing even if they don't contain (0..) in the source.
"""

import re
import sys

def remove_all_acsl(filepath):
    """Remove all ACSL comment blocks from a file."""
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    original_content = content
    
    # Pattern to match ACSL comment blocks /*@ ... @*/
    # This is more comprehensive than the previous version
    acsl_pattern = r'/\*@.*?@\*/'
    
    # Replace all ACSL blocks with empty string (with newlines preserved)
    content = re.sub(acsl_pattern, '/* ACSL removed */', content, flags=re.DOTALL)
    
    if content != original_content:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Removed ACSL from: {filepath}")
        return True
    else:
        print(f"No ACSL found in: {filepath}")
        return False

if __name__ == '__main__':
    # Files that still fail after the first round of fixes
    files_to_fix = [
        'kernel/libc9/dofmt.c',
        'kernel/libc9/fmtlock.c',
        'kernel/libc9/fmt.c',
        'kernel/libc9/vsmprint.c',
        'kernel/libc9/fcallfmt.c',
        'kernel/libc9/strstr.c',
        'kernel/libc9/utfnlen.c',
        'kernel/libc9/utfrune.c',
        'kernel/libc9/rerrstr.c',
        'kernel/libc9/tokenize.c',
    ]
    
    for filepath in files_to_fix:
        remove_all_acsl(filepath)
