import re

line = r"\item \textbf{Complexity : Higher}"
patterns = [
    r"Complexity",
    r"Complexity\s+:",
    r"\textbf",
    r"\textbf\{",
    r"\textbf\{Complexity",
    r"\textbf\{Complexity\s+:",
    r"\textbf\{([^}:]+?)\s+:",
]

for p in patterns:
    m = re.search(p, line)
    print(f"Pattern '{p}': {'MATCH' if m else 'NO MATCH'}")
