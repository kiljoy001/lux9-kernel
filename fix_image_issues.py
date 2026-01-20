import re
import os
import subprocess

# 1. Add required packages to preamble
with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

if "\\usepackage{float}" not in content:
    content = content.replace("\\usepackage{graphicx}", "\\usepackage{graphicx}\n\\usepackage{float}")

# 2. Fix Figure Placement ([h] -> [H])
# Regex replace \begin{figure}[h] or just \begin{figure} with \begin{figure}[H]
# Be careful not to double up.
content = re.sub(r"\\begin\{figure\}\s*(\[.*?\])?", r"\\begin{figure}[H]", content)

# 3. Constrain Image Size
# Use keepaspectratio and max sizes to prevent blowing up the page
# We used: \includegraphics[width=\textwidth]{...}
# Let's change to: \includegraphics[width=0.95\textwidth,height=0.45\textheight,keepaspectratio]{...}
# This prevents vertical overflow.
content = content.replace(r"\includegraphics[width=\textwidth]", r"\includegraphics[width=0.95\textwidth,height=0.45\textheight,keepaspectratio]")

with open("lux9_technical_manual.tex", "w") as f:
    f.write(content)

print("Updated LaTeX source.")

# 4. Re-convert images with safer settings to avoid artifacts
# Pattern: \includegraphics[...]{(docs/.*?\.pdf)}
matches = re.findall(r"\\includegraphics\[.*?\]\{(.*?\.pdf)\}", content)
print(f"Found {len(matches)} images to re-convert.")

for pdf_path in matches:
    svg_path = pdf_path.replace(".pdf", ".svg")
    if os.path.exists(svg_path):
        print(f"Re-converting {svg_path} (Safe Mode)...")
        # --export-text-to-path: converts text to curves (fixes font issues/black boxes)
        # --export-area-drawing: exports only the drawing area (tight crop)
        cmd = ["inkscape", svg_path, "--export-text-to-path", "--export-area-drawing", "-o", pdf_path]
        try:
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError as e:
            print(f"Failed to convert {svg_path}: {e}")

print("Done.")
