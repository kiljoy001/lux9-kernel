import re

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

# Pattern: Use 4 backslashes for textbf just to be safe based on debug
# \textbf{KEY : -> \textbf{KEY}:
pattern = r"\\textbf\{([^}:]+?)\s+:"
replacement = r"\textbf{\1}:"

# Note: In replacement string, \textbf means \textbf because it's not raw string?
# Wait, r"\textbf{\1}:" -> string \textbf{\1}:
# re.sub uses backslash escapes in replacement too? 
# re.sub replacement: \1 is group 1.
# \text... matches literal \text?
# Safest is to use literal function or 4 backslashes if needed.
# But standard python replacement string: r"\textbf" puts \textbf.

# Let's try raw replacement string.
replacement = r"\textbf{\1}:"

# We must verify if this matches.
# Let's test on memory string first.
test_line = r"\item \textbf{Complexity : Higher"
test_match = re.search(pattern, test_line)
if test_match:
    print(f"Sanity Check MATCH: {test_match.group(0)}")
else:
    print("Sanity Check NO MATCH")

new_content = re.sub(pattern, replacement, content)

if new_content != content:
    print("Made replacements.")
else:
    print("No replacements made.")

with open("lux9_technical_manual.tex", "w") as f:
    f.write(new_content)
