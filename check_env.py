import re

with open("lux9_technical_manual.tex", "r") as f:
    lines = f.readlines()

stack = []
for i, line in enumerate(lines):
    line_num = i + 1
    begins = re.findall(r"\\begin\{(.*?)\}", line)
    ends = re.findall(r"\\end\{(.*?)\}", line)
    
    for b in begins:
        stack.append((b, line_num))
    for e in ends:
        if stack:
            top_name, top_line = stack[-1]
            if top_name == e:
                stack.pop()
            else:
                print(f"Mismatch: \\end{{{e}}} at line {line_num} does not match \\begin{{{top_name}}} at line {top_line}")
                # Try to recover?
                stack.pop()
        else:
            print(f"\\end{{{e}}} at line {line_num} has no matching \\begin")

if stack:
    print(f"Unclosed environments: {len(stack)}")
    for name, line in stack:
        print(f"\\begin{{{name}}} at line {line}")
