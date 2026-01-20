import re

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

# Repair corruption from previous script
# \x08 is Backspace (\b), \x0c is Form Feed (\f)
content = content.replace("\x08egin", "\\begin")
content = content.replace("\x0cbox", "\\fbox")
content = content.replace("\x08end", "\\end")

# Now apply the fixes intended previously:
# 1. Fix mismatched verbatim/lstlisting at line ~4096
# We look for \begin{verbatim} followed by box drawing
# We can just replace ALL \begin{verbatim} that are followed eventually by \end{lstlisting}?
# Or just replace the specific one.
# Since we know the context (User Process diagram), let's find it.

# Look for \begin{verbatim} ... \end{lstlisting} mismatch
# Regex to find unclosed verbatim? No, regex on large file is hard.
# Let's iterate lines.
lines = content.splitlines()
new_lines = []
in_mismatch_block = False
for i, line in enumerate(lines):
    if "\\begin{verbatim}" in line:
        # Check ahead for box drawing
        is_box = False
        for j in range(1, 5):
            if i+j < len(lines) and "User Process" in lines[i+j]:
                is_box = True
                break
        
        if is_box:
            # Check if it ends with lstlisting
            # Actually, just blindly switch to lstlisting is safe for box drawing
            line = line.replace("\\begin{verbatim}", "\\begin{lstlisting}")

    # Also fix SVGs - existing ones might be fixed by the replace above (repairing corruption)
    # But checking if any remain un-replaced?
    # The previous script replaced them with corrupted strings.
    # The replace() calls above fixed the corruption to valid \begin{center}...
    # So SVGs should be fine now (as placeholders).
    
    new_lines.append(line)

content = "\n".join(new_lines) + "\n"

with open("lux9_technical_manual.tex", "w") as f:
    f.write(content)
