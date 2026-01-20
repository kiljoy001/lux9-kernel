import re

with open("lux9_technical_manual.tex", "r") as f:
    lines = f.readlines()

new_lines = []
for i, line in enumerate(lines):
    # Fix extbf typos
    if r"\textbfextbf" in line:
        line = line.replace(r"\textbfextbf", r"\textbf")
    if "\t" in line and "extbf" in line:
        line = line.replace("\textbf", r"\textbf")
        # Handle case where tab is interpreted as literal tab char in string search
        # If line contains literal tab char followed by extbf
        line = re.sub(r"\textbf", r"\\textbf", line)
    
    # Check for stray \end{lstlisting}
    # We identified them at lines (1-indexed): 355, 383, 879
    # But line numbers are from file view.
    # We can detect them by context: "\end{lstlisting}" on a line by itself (with whitespace)
    # AND likely causing imbalance.
    # But we can't easily know if it's stray without parsing.
    # However, the pattern we saw: "\end{lstlisting}" after a paragraph, followed by section or figure.
    # And we know there are exactly 3 extra ends.
    # So we can just drop the specific ones if we match context.
    
    # Context 1: after "caching properties." (line ~353)
    # Context 2: after "direct physical memory access." (line ~381)
    # Context 3: after "sophisticated tracking and validation." (line ~877)
    
    is_stray = False
    if "\\end{lstlisting}" in line:
        # Check previous line (ignoring empty lines)
        # This is hard in a streaming loop.
        # But we know exact line numbers from previous tool output for the UNMODIFIED file.
        # Line 355 (0-indexed 354)
        # Line 383 (0-indexed 382)
        # Line 879 (0-indexed 878)
        
        # Verify
        if i == 354: # Line 355
            # Double check content
            if "end{lstlisting}" in line:
                is_stray = True
        elif i == 382: # Line 383
            if "end{lstlisting}" in line:
                is_stray = True
        elif i == 878: # Line 879
            if "end{lstlisting}" in line:
                is_stray = True
                
    if not is_stray:
        new_lines.append(line)
    else:
        print(f"Removed stray end at line {i+1}")

with open("lux9_technical_manual.tex", "w") as f:
    f.writelines(new_lines)

print("Typos fixed.")
