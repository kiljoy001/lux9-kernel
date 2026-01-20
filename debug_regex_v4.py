import re

line = r"\item \textbf{Complexity : Higher}"
patterns = [
    r"\textbf",   # matches \ + textbf ? OR maybe \t is TAB? 
                   # \t in regex is TAB.
                   # \t matches \t (backslash t).
    r"\\textbf", # Matches literal backslash + textbf
]

for p in patterns:
    m = re.search(p, line)
    print(f"Pattern '{p}': {'MATCH' if m else 'NO MATCH'}")
