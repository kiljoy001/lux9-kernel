import sys
import re

def strip_acsl(file_path):
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Remove /*@ ... */ blocks (multiline)
    # Using non-greedy match .*? with DOTALL
    content = re.sub(r'/\*@.*?\*/', '', content, flags=re.DOTALL)
    
    # Remove //@ ... lines
    content = re.sub(r'//@.*', '', content)
    
    with open(file_path, 'w') as f:
        f.write(content)
    print(f"Stripped ACSL from {file_path}")

files = [
    "kernel/9front-port/devcons_minimal.c",
    "kernel/9front-port/devmnt.c",
    "kernel/9front-port/devram.c",
    "kernel/9front-port/devpipe.c",
    "kernel/9front-port/sysfile.c"
]

for f in files:
    strip_acsl(f)
