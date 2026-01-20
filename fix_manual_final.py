import re

with open("lux9_technical_manual.tex", "r") as f:
    lines = f.readlines()

new_lines = []
for i, line in enumerate(lines):
    # Fix the mismatched environment at line 4096 (approx)
    # We look for \begin{verbatim} that is followed closely by the box drawing chars
    if "\begin{verbatim}" in line:
        # Check if next few lines have box drawing chars, unique to this block
        context = "".join(lines[i+1:i+5])
        if "User Process" in context or "Blind Ledger" in context:
            line = line.replace("\begin{verbatim}", "\begin{lstlisting}")
    
    # Fix SVG includes
    # \includegraphics[width=\textwidth]{docs/diagrams/security_layers.svg}
    # -> \fbox{SVG Image: docs/diagrams/security_layers.svg}
    if "\includegraphics" in line and ".svg" in line:
        # Extract filename for the label
        match = re.search(r"\{([^}]+)\}", line)
        if match:
            fname = match.group(1)
            line = "\begin{center}\fbox{SVG Image Placeholder: " + fname + "}\end{center}\n"
        else:
            line = "\begin{center}\fbox{SVG Image Placeholder}\end{center}\n"
            
    new_lines.append(line)

with open("lux9_technical_manual.tex", "w") as f:
    f.writelines(new_lines)

