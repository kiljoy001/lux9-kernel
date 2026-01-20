with open("lux9_technical_manual.tex", "r") as f:
    lines = f.readlines()

stack = []
for i, line in enumerate(lines):
    line_num = i + 1
    for char in line:
        if char == '{':
            stack.append(line_num)
        elif char == '}':
            if stack:
                stack.pop()
            else:
                print(f"Unmatched '}}' at line {line_num}")

if stack:
    print(f"Unmatched '{{' found. Count: {len(stack)}")
    for s in stack[-10:]: # Show last 10
        print(f"Line {s}")
