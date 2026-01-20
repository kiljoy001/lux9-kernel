import sys

def repair_content(content):
    # \x08 is Backspace. Was intended to be \begin or \bf or something starting with b.
    # Context usually: \begin{...}
    # It seems we only corrupted lines we touched in the previous script.
    # The previous script touched lines with \begin{verbatim} (replaced with lstlisting)
    # AND lines with \includegraphics (replaced with \begin{center}\fbox...)
    
    # \begin{center} -> \b (BS) egin{center}
    # \fbox -> \f (FF) box
    # \end{center} -> \e (valid) nd{center} -- wait, \e is not an escape?
    # Actually \e is NOT a python escape for ESC. \x1b is.
    # So \end stayed \end IF the shell passed \end.
    
    # REPAIR STRATEGY:
    # 1. Replace \x08egin with \\begin
    # 2. Replace \x0cbox with \\fbox
    # 3. Replace \t (Tab) + extbf with \\textbf
    # 4. Any other \b or \f or \t?
    
    # Check for \textbf corruption which caused 'extbf'
    # 'extbf' (Tab + extbf)
    
    new_chars = []
    i = 0
    n = len(content)
    while i < n:
        c = content[i]
        
        # Check for Backspace \x08 -> \begin
        if c == '\x08':
            # Peek ahead to see if it's 'egin'
            if content[i+1:i+5] == 'egin':
                new_chars.append('\\begin')
                i += 1
                continue
            else:
                # Just a random backspace? Replace with \b just in case? 
                # Or refers to \bf?
                # Assume it's a generic \b re-insertion
                new_chars.append('\\b')
        
        # Check for FormFeed \x0c -> \fbox
        elif c == '\x0c':
            if content[i+1:i+4] == 'box':
                new_chars.append('\\fbox')
                i += 1
                continue
            else:
                new_chars.append('\\f')
        
        # Check for Tab \t -> \textbf or just \t
        elif c == '\t':
            if content[i+1:i+6] == 'extbf':
                new_chars.append('\\textbf')
                i += 1
                continue
            else:
                # Keep legitimate tabs
                new_chars.append('\t')
                
        # Check for other corruptions? 
        # previous script used: "\\begin{lstlisting}"
        # \b -> BS.
        # So \begin{lstlisting} became \x08egin{lstlisting}.
        
        else:
            new_chars.append(c)
        
        i += 1
            
    return "".join(new_chars)

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

fixed = repair_content(content)

with open("lux9_technical_manual.tex", "w") as f:
    f.write(fixed)

print("Repair complete.")
