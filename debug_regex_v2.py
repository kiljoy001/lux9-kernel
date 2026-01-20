import re

line = r"\item \textbf{Complexity : Higher but with mathematical verification"
pattern = r"\textbf\{([^}:]+?)\s+:"

match = re.search(pattern, line)
if match:
    print(f"MATCH: {match.group(0)} -> Group 1: {match.group(1)}")
else:
    print("NO MATCH")
