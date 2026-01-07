## CRITICAL: PLAN 9 REQUIRES NEWLINES AT END OF FILES

**THIS IS NOT OPTIONAL - PLAN 9 WILL FAIL WITHOUT NEWLINES**

### EVERY Plan 9 Script/File MUST End With a Newline

**SYMPTOMS OF MISSING NEWLINE:**
- `token EOF: syntax error` 
- Script stops executing at last line
- Commands don't run properly

**MANDATORY RULE:**
1. **ALWAYS** add a blank line at the end of EVERY rc script
2. **ALWAYS** add a newline at the end of EVERY text file  
3. **NEVER** trust that Write tool adds it - explicitly include it

**HOW TO ENSURE NEWLINE:**
```python
# When writing files, ALWAYS end content with \n:
content = "#!/bin/rc\ncommand1\ncommand2\n"  # <- NOTE THE \n AT END
```

**CHECK YOURSELF BEFORE EVERY FILE WRITE:**
"Did I add \n at the end of this file content?"
If NO → ADD IT NOW

**This is Plan 9, not Linux. Files without trailing newlines WILL BREAK.**

Remember: That "token EOF: syntax error" you keep seeing? 
IT'S BECAUSE YOU FORGOT THE FUCKING NEWLINE AGAIN.