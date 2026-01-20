import os
import subprocess
import re
import sys

# 1. Find all SVGs referenced in the manual or just all in docs?
# Let's find all in docs to be safe, or scan the TEX file.
# Scanning TEX is better to know what we actually need.
# But we changed the TEX to placeholders.
# The placeholders contain the path: "SVG Image Placeholder: docs/diagrams/foo.svg"

tex_file = "lux9_technical_manual.tex"
with open(tex_file, "r") as f:
    content = f.read()

# Find paths
# Pattern: \fbox{SVG Image Placeholder: (.*?)}
# Note: We added spaces.
placeholder_pattern = r"\\fbox\{SVG Image Placeholder: (.*?\.svg)\}"
matches = re.findall(placeholder_pattern, content)

print(f"Found {len(matches)} SVG references to process.")

for svg_path in matches:
    svg_path = svg_path.strip()
    # Check if exists
    if not os.path.exists(svg_path):
        print(f"Warning: {svg_path} not found!")
        continue
    
    pdf_path = svg_path.replace(".svg", ".pdf")
    
    # Needs update?
    needs_convert = True
    if os.path.exists(pdf_path):
        if os.path.getmtime(pdf_path) > os.path.getmtime(svg_path):
            needs_convert = False
    
    if needs_convert:
        print(f"Converting {svg_path} to {pdf_path}...")
        # Inkscape 1.0+ CLI: inkscape filename.svg --export-filename=filename.pdf
        # Or: inkscape input.svg -o output.pdf
        cmd = ["inkscape", svg_path, "-o", pdf_path]
        try:
            subprocess.check_call(cmd)
        except subprocess.CalledProcessError as e:
            print(f"Error converting {svg_path}: {e}")
    else:
        print(f"Skipping {svg_path} (up to date)")

# 2. Update TeX file
# Revert: \begin{center}\fbox{SVG Image Placeholder: PATH}\end{center}
# To: \includegraphics[width=\textwidth]{PATH_PDF}

# Construct regex to match the block
# We used: line = "\\begin{center}\\fbox{SVG Image Placeholder: " + fname + "}\\end{center}\n"
# Regex must match that line.

print("Updating LaTeX source...")
new_lines = []
lines = content.splitlines()
for line in lines:
    match = re.search(placeholder_pattern, line)
    if match:
        svg_path = match.group(1).strip()
        pdf_path = svg_path.replace(".svg", ".pdf")
        
        # We need to construct the includegraphics command.
        # Ensure we replace the WHOLE center/fbox block if it's on one line
        # Our previous script put it on one line.
        
        # If the line looks like our placeholder line:
        if "\\fbox{SVG Image Placeholder:" in line:
            # Replace the whole visual block with includegraphics
            # We wrapped it in \begin{center}...\end{center}
            # Standard latex for float is:
            # \centering \includegraphics... inside figure
            # But the figure environment is outside this line usually.
            # Let's check context.
            # The previous script:
            # \begin{figure}[h]
            # \centering
            # \begin{center}\fbox{...}\end{center}
            # \caption...
            
            # The \centering is redundant if we have \begin{center}, but okay.
            # We can replace the \begin{center}...\end{center} with \includegraphics...
            
            # Extract path from match again to be sure
            new_line = f"\\includegraphics[width=\\textwidth]{{{pdf_path}}}"
            new_lines.append(new_line)
        else:
            # Just some random mention? Unlikely given the regex match.
            new_lines.append(line)
    else:
        new_lines.append(line)

final_content = "\n".join(new_lines) + "\n"

with open("lux9_technical_manual_inkscape.tex", "w") as f:
    f.write(final_content)

# Overwrite original
with open(tex_file, "w") as f:
    f.write(final_content)

print("Done.")
