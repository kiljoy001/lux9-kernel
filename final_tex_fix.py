import re

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

# 1. Fix Preamble
content = content.replace("fancyhdr}\\n\\setlength{\\headheight}{14pt}\\n\\addtolength{\\topmargin}{-2pt}", 
                          "fancyhdr}\n\\setlength{\\headheight}{14pt}\n\\addtolength{\\topmargin}{-2pt}")

# 2. Split Exchange Page Figure
# Find the specific block
# We know the label is fig:exchange_pages
lines = content.splitlines()
start_idx = -1
end_idx = -1
for i, line in enumerate(lines):
    if "\\label{fig:exchange_pages}" in line:
        # Search upwards for \begin{figure}
        for j in range(i, i-10, -1):
            if "\\begin{figure}" in lines[j]:
                start_idx = j
                break
        # Search downwards for \end{figure}
        for j in range(i, i+10):
            if "\\end{figure}" in lines[j]:
                end_idx = j
                break
        break

if start_idx != -1 and end_idx != -1:
    print(f"Replacing lines {start_idx}-{end_idx}")
    new_section = r"""\begin{figure}[H]
\centering
\includegraphics[width=0.95\textwidth,height=0.40\textheight,keepaspectratio]{docs/exchange_page_mechanism.pdf}
\caption{Exchange Page Zero-Copy Bridge - Shared mapping between Ring 3 and Ring 0}
\label{fig:exchange_mechanism}
\end{figure}

\begin{figure}[H]
\centering
\includegraphics[width=0.95\textwidth,height=0.40\textheight,keepaspectratio]{docs/exchange_page_pool.pdf}
\caption{Global Exchange Pool - 64-page resource management and process binding}
\label{fig:exchange_pool}
\end{figure}"""
    
    # Replace the range
    new_lines = lines[:start_idx] + [new_section] + lines[end_idx+1:]
    content = "\n".join(new_lines)
else:
    print("Could not find exchange_pages figure block")

with open("lux9_technical_manual.tex", "w") as f:
    f.write(content)
