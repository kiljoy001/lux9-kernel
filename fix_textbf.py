import re

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

# Replace any malformed \textbf that might have hidden chars
# Pattern: \textbf followed by { then text then }
# We'll normalize it to exactly \textbf{TEXT}
def clean_textbf(match):
    text = match.group(1)
    return f"\\textbf{{{text}}}"

# We need to be careful with nested ones, but let's try shallow first
content = re.sub(r"\\textbf\s*\{([^{}]*)\}", clean_textbf, content)

with open("lux9_technical_manual.tex", "w") as f:
    f.write(content)
