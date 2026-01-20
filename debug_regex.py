import re

line = r"\item \textbf{Complexity : Higher but with mathematical verification"
pattern = r"\textbf\{([^}:]+)\s+:"

match = re.search(pattern, line)
if match:
    print(f"MATCH: {match.group(0)}")
else:
    print("NO MATCH")
    # Debug parts
    print(f"Pattern: {pattern}")
    print(f"Line: {line}")
