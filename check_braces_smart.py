import re

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

# Remove lstlisting and verbatim blocks
# We need to handle them carefully.
def remove_blocks(text, start_tag, end_tag):
    pattern = re.escape(start_tag) + r".*?" + re.escape(end_tag)
    return re.sub(pattern, " ", text, flags=re.DOTALL)

clean_content = content
clean_content = remove_blocks(clean_content, "\\begin{lstlisting}", "\\end{lstlisting}")
clean_content = remove_blocks(clean_content, "\\begin{verbatim}", "\\end{verbatim}")

# Now check braces in clean_content
stack = []
for i, char in enumerate(clean_content):
    if char == '{':
        # Find line number in original? Unlikely to work easily with sub.
        # Let's just count.
        stack.append(i)
    elif char == '}':
        if stack:
            stack.pop()
        else:
            print(f"Unmatched '}}' at position {i}")

if stack:
    print(f"Unmatched '{{' found. Count: {len(stack)}")
    # Find line number for each
    lines = content.splitlines()
    for pos in stack:
        # This pos is in clean_content, not original.
        # This approach is hard.
        pass

# Better approach: parse line by line and track in_block
lines = content.splitlines()
in_block = False
stack = []
for i, line in enumerate(lines):
    line_num = i + 1
    if "\\begin{lstlisting}" in line: in_block = True
    if "\\begin{verbatim}" in line: in_block = True
    
    if not in_block:
        for char in line:
            if char == '{':
                stack.append(line_num)
            elif char == '}':
                if stack:
                    stack.pop()
                else:
                    print(f"Unmatched '}}' at line {line_num}")
    
    if "\\end{lstlisting}" in line: in_block = False
    if "\\end{verbatim}" in line: in_block = False

if stack:
    print(f"Unmatched '{{' found. Count: {len(stack)}")
    for s in stack:
        print(f"Line {s}")
