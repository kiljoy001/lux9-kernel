import re

with open("lux9_technical_manual.tex", "r") as f:
    lines = f.readlines()

in_block = False
for i, line in enumerate(lines):
    line_num = i + 1
    if "\\begin{lstlisting}" in line or "\\begin{verbatim}" in line:
        in_block = True
    if "\\end{lstlisting}" in line or "\\end{verbatim}" in line:
        in_block = False
        continue
    
    if not in_block:
        if "%" in line:
            # Check if escaped \%
            if not re.search(r"\\%", line):
                print(f"Literal '%' at line {line_num}: {line.strip()}")
