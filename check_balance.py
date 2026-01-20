import re

with open("lux9_technical_manual.tex", "r") as f:
    lines = f.readlines()

stack = []
errors = []

for i, line in enumerate(lines):
    line_num = i + 1
    
    # Simple regex for begin/end
    # Note: Handles multiple on one line if they don't nest on same line?
    # LaTeX usually works clearly.
    # regex iter for tags?
    
    tags = re.finditer(r"\\(begin|end)\{([a-zA-Z0-9\*]+)\}", line)
    for match in tags:
        cmd = match.group(1)
        env = match.group(2)
        
        if cmd == "begin":
            stack.append((env, line_num))
        elif cmd == "end":
            if not stack:
                print(f"Line {line_num}: Extra \\end{{{env}}}")
            else:
                last_env, last_line = stack[-1]
                if last_env == env:
                    stack.pop()
                else:
                    print(f"Line {line_num}: Mismatch \\end{{{env}}}. Expected \\end{{{last_env}}} (started line {last_line})")
                    # Try to recover? 
                    # If we find the env deeper in stack?
                    # For now just pop and warn
                    if stack and stack[-1][0] == env:
                         stack.pop()
                    else:
                         # Assume missing begin?
                         pass

if stack:
    print("Unclosed environments:")
    for env, line in stack:
        print(f"  {env} at line {line}")

# Also check for corruption
found_corruption = False
for i, line in enumerate(lines):
    if '\x08' in line:
        print(f"Line {i+1}: Contains \\x08 (Backspace)")
        found_corruption = True
    if '\t' in line and 'extbf' in line:
        print(f"Line {i+1}: Contains Tab+extbf potential typo")

