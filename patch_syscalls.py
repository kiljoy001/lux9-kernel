import re
import os

files = ['kernel/9front-port/sysfile.c', 'kernel/9front-port/sysproc.c']

header = """
/* Local Plan 9 Syscall ABI fix */
#include <u.h>
typedef ulong *syscall_va_list;
#define SYSCALL_ARG(list, type) (*(type*)((list)++))
/* va_list macro removed to prevent stdarg.h conflict */
#define va_start(list, start) ((void)0)
#define va_end(list) ((void)0)
"""

header_marker = "/* Local Plan 9 Syscall ABI fix */"

for fpath in files:
    with open(fpath, 'r') as f:
        content = f.read()

    # Clean existing headers
    while header_marker in content:
         start = content.find(header_marker)
         end = content.find("typedef ulong *syscall_va_list;", start)
         if end != -1:
             end = content.find("\n", end)
             end_marker = "#define va_end(list) ((void)0)"
             real_end = content.find(end_marker, start)
             if real_end != -1:
                 end = real_end + len(end_marker)
             else:
                 pass
             content = content[:start] + content[end+1:]
         else:
             break

    # Inject new header
    m = re.search(r"^#include .*$", content, re.MULTILINE)
    if m:
        pos = m.end() + 1
        content = content[:pos] + header + content[pos:]
    else:
        content = header + content

    # REPAIR previously patched signatures
    # Look for: va_list list = (va_list)list_void;
    # Replace with: syscall_va_list list = (syscall_va_list)list_void;
    repair_regex = r"va_list\s+(\w+)\s*=\s*\(va_list\)(\w+);"
    content = re.sub(repair_regex, r"syscall_va_list \1 = (syscall_va_list)\2;", content)

    # Apply Original Patch Logic (for unpatched functions)
    regex = r"(uintptr\s+)(sys\w+)\s*\(\s*va_list\s*(\w*)\s*\)\s*\{"
    def replacement(m):
        func_prefix = m.group(1) 
        func_name = m.group(2)
        arg_name = m.group(3)
        if not arg_name: arg_name = "list"
        return f"{func_prefix}{func_name}(void *{arg_name}_void)\n{{\n\tsyscall_va_list {arg_name} = (syscall_va_list){arg_name}_void;"

    new_content = re.sub(regex, replacement, content, flags=re.MULTILINE)
    
    # Replace va_arg -> SYSCALL_ARG
    new_content = re.sub(r"va_arg\s*\(\s*(\w+)\s*,\s*([^)]+)\s*\)", r"SYSCALL_ARG(\1, \2)", new_content)
    
    with open(fpath, 'w') as f:
        f.write(new_content)
    print(f"Patched {fpath}")
