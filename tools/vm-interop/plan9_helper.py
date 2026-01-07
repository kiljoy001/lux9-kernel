#!/usr/bin/env python3
"""
Plan 9 Helper - Makes Plan 9 more user-friendly
Uses Solr-indexed docs and code to provide instant help
"""

import requests
import sys
import json

SOLR_URL = "http://localhost:8983/solr/plan9"

def search_help(query):
    """Search Plan 9 docs and code for help"""
    # Search documentation first
    params = {
        "q": f"content:{query}",
        "fq": "type:section OR type:page",
        "fl": "chapter,section,content",
        "rows": 3,
        "wt": "json"
    }
    
    r = requests.get(f"{SOLR_URL}/select", params=params)
    docs = r.json()["response"]["docs"]
    
    if docs:
        print(f"\n📚 Documentation ({len(docs)} results):")
        for doc in docs[:2]:
            chapter = doc.get("chapter", "")
            section = doc.get("section", "")
            content = doc.get("content", [""])[0] if isinstance(doc.get("content"), list) else doc.get("content", "")
            # Extract relevant snippet
            idx = content.lower().find(query.lower())
            if idx > -1:
                start = max(0, idx - 100)
                end = min(len(content), idx + 200)
                snippet = content[start:end].replace("\n", " ")
                print(f"\n{chapter} - {section}:")
                print(f"...{snippet}...")
    
    # Search code examples
    params["fq"] = "type:source_code"
    params["fl"] = "file_path,content"
    
    r = requests.get(f"{SOLR_URL}/select", params=params)
    code_docs = r.json()["response"]["docs"]
    
    if code_docs:
        print(f"\n💻 Code Examples ({len(code_docs)} files):")
        for doc in code_docs[:2]:
            print(f"  - {doc.get('file_path', 'unknown')}")

def translate_command(bash_cmd):
    """Translate bash/Linux commands to Plan 9 equivalents"""
    translations = {
        "ls": "ls",
        "cd": "cd",
        "pwd": "pwd",
        "cat": "cat",
        "grep": "grep",
        "find": "du -a | grep",
        "ps": "ps",
        "kill": "kill",
        "man": "man",
        "vim": "sam or acme",
        "emacs": "acme",
        "bash": "rc",
        "sh": "rc",
        "export": "variable=value (no export needed)",
        "source": ". (dot)",
        "apt": "no package manager - compile from source",
        "sudo": "no sudo - use cpu command for remote execution"
    }
    
    for linux_cmd, plan9_cmd in translations.items():
        if linux_cmd in bash_cmd:
            return f"In Plan 9: {bash_cmd.replace(linux_cmd, plan9_cmd)}"
    
    return f"No direct translation, try searching: {bash_cmd}"

def generate_rc_script(description):
    """Generate rc script template based on description"""
    # Search for similar scripts
    params = {
        "q": f"content:{description} AND file_type:rc_script",
        "fl": "file_path,content",
        "rows": 1,
        "wt": "json"
    }
    
    r = requests.get(f"{SOLR_URL}/select", params=params)
    docs = r.json()["response"]["docs"]
    
    if docs:
        print(f"Found similar script: {docs[0].get('file_path', '')}")
        return docs[0].get("content", [""])[0][:500] if docs else ""
    
    # Generate basic template
    if "loop" in description.lower():
        return """#!/bin/rc
# Loop template
while(test 1){
    # Your code here
    sleep 1
}"""
    elif "check" in description.lower() or "if" in description.lower():
        return """#!/bin/rc
# Conditional template
if(test -f /path/to/file){
    echo File exists
}
if not {
    echo File does not exist
}"""
    else:
        return """#!/bin/rc
# Basic rc script
rfork e
# Your code here"""

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Plan 9 Helper - Making Plan 9 user-friendly")
        print("\nUsage:")
        print("  ./plan9_helper.py search <query>     - Search docs and code")
        print("  ./plan9_helper.py translate <cmd>    - Translate Linux to Plan 9")
        print("  ./plan9_helper.py script <desc>      - Generate rc script template")
        print("\nExamples:")
        print("  ./plan9_helper.py search 'while loop'")
        print("  ./plan9_helper.py translate 'find . -name *.c'")
        print("  ./plan9_helper.py script 'monitor file changes'")
        sys.exit(0)
    
    cmd = sys.argv[1]
    
    if cmd == "search" and len(sys.argv) > 2:
        search_help(" ".join(sys.argv[2:]))
    elif cmd == "translate" and len(sys.argv) > 2:
        result = translate_command(" ".join(sys.argv[2:]))
        print(result)
    elif cmd == "script" and len(sys.argv) > 2:
        template = generate_rc_script(" ".join(sys.argv[2:]))
        print(template)
    else:
        print(f"Unknown command: {cmd}")