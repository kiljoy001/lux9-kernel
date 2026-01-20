import re

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

# Pattern: \textbf{TEXT : -> \textbf{TEXT}:
# We capture TEXT which should not contain } or :
pattern = r"\textbf\{([^}:]+)\s+:"
# Replacement: \textbf{TEXT}:
replacement = r"\textbf{\1}:"

new_content = re.sub(pattern, replacement, content)

# Check if changes were made
if new_content != content:
    print("Made replacements.")
else:
    print("No replacements made.")

with open("lux9_technical_manual.tex", "w") as f:
    f.write(new_content)
