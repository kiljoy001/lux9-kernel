import re

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

# Non-greedy match for key
pattern = r"\textbf\{([^}:]+?)\s+:"
replacement = r"\textbf{\1}:"

new_content = re.sub(pattern, replacement, content)

if new_content != content:
    print("Made replacements.")
else:
    print("No replacements made.")

with open("lux9_technical_manual.tex", "w") as f:
    f.write(new_content)
