#!/usr/bin/env python3
import sys
import subprocess
import os

def compile_ssa(input_file, output_file, qbe_path):
    with open(input_file, 'r') as f:
        lines = f.readlines()

    globals_content = []
    functions = []
    current_func = []
    in_func = False

    for line in lines:
        if line.startswith('export function') or line.startswith('function'):
            in_func = True
            current_func = [line]
        elif in_func:
            current_func.append(line)
            if line.strip() == '}':
                in_func = False
                functions.append(current_func)
                current_func = []
        else:
            globals_content.append(line)

    # Compile globals (data definitions)
    # We pass them to QBE to check validity, and output them to init.s
    # QBE might require a function to be present? No.
    # But QBE outputs assembly. Data sections are assembly.
    
    final_asm = []
    
    # Process Globals
    # We can just write globals directly to assembly??
    # No, QBE translates `data` to `.data`.
    # So we run globals through QBE.
    
    if globals_content:
        # Create a dummy function to make QBE happy if needed? 
        # QBE is fine with just data.
        proc = subprocess.Popen([qbe_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        out, err = proc.communicate(input="".join(globals_content))
        if proc.returncode == 0:
            final_asm.append(out)
        else:
            print(f"Error compiling globals: {err}")
            # If globals fail, we are in trouble. But usually they are simple.

    # Process functions
    success_count = 0
    fail_count = 0
    
    for i, func_lines in enumerate(functions):
        func_content = "".join(func_lines)
        func_name = func_lines[0].split('(')[0].split()[-1] # extract name roughly
        
        proc = subprocess.Popen([qbe_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        out, err = proc.communicate(input=func_content)
        
        if proc.returncode == 0:
            # Mangle local labels to prevent collisions
            # QBE generates .L... for local blocks/labels. 
            # We prefix them with a unique ID per function.
            # Regex: replace .L<word> with .L<func_idx>_<word>
            import re
            
            # Note: We must replace both definitions (.Lfoo:) and usages (jmp .Lfoo)
            # We must be careful not to mangle non-labels. 
            # In QBE output, local labels start with .L
            # e.g. .Lbb1:
            # jnz .Lbb1
            
            mangle_prefix = f".L{i}_"
            mangled_out = re.sub(r'(\.L[a-zA-Z0-9_]+)', f"{mangle_prefix}\\1", out)
            
            final_asm.append(mangled_out)
            success_count += 1
        else:
            print(f"Skipping function {func_name}: QBE Error (Validation failed)")
            # print(err) # Optional verbose
            fail_count += 1

    print(f"Compiled {success_count} functions. Skipped {fail_count} functions.")

    with open(output_file, 'w') as f:
        f.write("".join(final_asm))

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: compile_ssa.py <input.ssa> <output.s> <qbe_path>")
        sys.exit(1)
    
    compile_ssa(sys.argv[1], sys.argv[2], sys.argv[3])
