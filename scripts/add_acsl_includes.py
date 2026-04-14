#!/usr/bin/env python3
"""
Batch fix ACSL contracts in all problematic files with machine-realistic bounds.
"""

import os

# Template for string functions that take a format string
FMT_STRING_CONTRACT = '''/*@
  @ requires \\valid_read(fmt + (0..ACSL_MAX_FMT_LEN-1));
  @ requires \\exists integer n; 0 <= n < ACSL_MAX_FMT_LEN && fmt[n] == '\\0';
  @ assigns \\nothing;
  @ ensures \\result >= -1;
  @*/'''

# Template for string comparison functions
STR_CMP_CONTRACT = '''/*@
  @ requires \\valid_read(s1 + (0..ACSL_MAXSTR-1));
  @ requires \\valid_read(s2 + (0..ACSL_MAXSTR-1));
  @ requires \\exists integer k; 0 <= k < ACSL_MAXSTR && s1[k] == '\\0';
  @ requires \\exists integer j; 0 <= j < ACSL_MAXSTR && s2[j] == '\\0';
  @ requires n >= 0 && n <= ACSL_MAXSTR;
  @ assigns \\nothing;
  @ ensures \\result >= -1 && \\result <= 1;
  @*/'''

# Template for string to unsigned long
STRTOUL_CONTRACT = '''/*@
  @ requires \\valid_read(s + (0..ACSL_MAXSTR-1));
  @ requires \\exists integer k; 0 <= k < ACSL_MAXSTR && s[k] == '\\0';
  @ requires base >= 0 && base <= 36;
  @ assigns \\nothing;
  @ ensures \\result <= ACSL_MAX_UINT64;
  @*/'''

def add_include_if_missing(filepath):
    """Add acsl_bounds.h include if not present."""
    with open(filepath, 'r') as f:
        content = f.read()
    
    if 'acsl_bounds.h' not in content:
        # Find first #include and add after it
        lines = content.split('\n')
        for i, line in enumerate(lines):
            if line.strip().startswith('#include'):
                lines.insert(i+1, '#include "acsl_bounds.h"')
                break
        content = '\n'.join(lines)
        
        with open(filepath, 'w') as f:
            f.write(content)
        return True
    return False

# Add includes to all files that need them
files_needing_bounds = [
    'kernel/libc9/fmtprint.c',
    'kernel/libc9/vseprint.c',
    'kernel/libc9/vsnprint.c',
    'kernel/libc9/sprint.c',
    'kernel/libc9/cistrncmp.c',
    'kernel/libc9/strtoul.c',
    'kernel/libc9/fmtquote.c',
]

for filepath in files_needing_bounds:
    if os.path.exists(filepath):
        if add_include_if_missing(filepath):
            print(f"Added acsl_bounds.h to: {filepath}")
        else:
            print(f"Already has include: {filepath}")
