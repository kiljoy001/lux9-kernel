#!/usr/bin/env python3
"""
Plan 9 Remote Management System
Uses the autorun.rc script to execute commands and retrieve output
"""

import os
import sys
import time
import json
import requests

INTEROP_DIR = "/home/scott/Repo/VM-Interop"
SOLR_URL = "http://localhost:8983/solr/plan9"

class Plan9Remote:
    def __init__(self):
        self.cmd_file = os.path.join(INTEROP_DIR, "command.txt")
        self.out_file = os.path.join(INTEROP_DIR, "output.txt")
    
    def execute(self, command, timeout=5):
        """Execute command in 9front"""
        # Write command
        with open(self.cmd_file, 'w') as f:
            f.write(command)
        
        print(f"→ {command}")
        
        # Wait for execution
        time.sleep(timeout)
        
        # Read output
        if os.path.exists(self.out_file):
            with open(self.out_file, 'r') as f:
                output = f.read()
            os.remove(self.out_file)
            return output
        return "No output (autorun.rc may not be running)"
    
    def search_command(self, query):
        """Search for command usage in indexed docs/code"""
        params = {
            "q": f"content:{query}",
            "fq": "type:manual OR file_type:rc_script",
            "fl": "chapter,section,file_path,content",
            "rows": 3,
            "wt": "json"
        }
        
        r = requests.get(f"{SOLR_URL}/select", params=params)
        docs = r.json()["response"]["docs"]
        
        results = []
        for doc in docs:
            if "chapter" in doc:
                results.append(f"Man: {doc['chapter']} - {doc.get('section', '')}")
            elif "file_path" in doc:
                results.append(f"Script: {doc['file_path']}")
        return results
    
    def system_info(self):
        """Get 9front system information"""
        commands = [
            ("Kernel", "cat /dev/sysname"),
            ("Memory", "cat /dev/swap"),
            ("Processes", "ps | wc -l"),
            ("Network", "cat /net/ndb"),
            ("Mounted", "ns | head -5")
        ]
        
        info = {}
        for name, cmd in commands:
            output = self.execute(cmd, timeout=2)
            info[name] = output.strip()
        return info
    
    def compile_vmx(self):
        """Compile and install VMX"""
        cmd = "cd /sys/src/cmd/vmx && mk clean && mk install && echo 'VMX rebuilt'"
        return self.execute(cmd, timeout=10)
    
    def search_code(self, pattern):
        """Search code in 9front"""
        cmd = f"grep -n '{pattern}' /sys/src/cmd/vmx/*.c | head -10"
        return self.execute(cmd)

def interactive_shell():
    """Interactive shell for 9front"""
    p9 = Plan9Remote()
    
    print("Plan 9 Remote Shell")
    print("Commands: exit, help, search <query>, info")
    print("-" * 40)
    
    while True:
        try:
            cmd = input("9> ").strip()
            
            if cmd == "exit":
                break
            elif cmd == "help":
                print("Direct commands are sent to 9front")
                print("Special commands:")
                print("  info     - System information")
                print("  search   - Search docs/code")
                print("  compile  - Rebuild VMX")
            elif cmd.startswith("search "):
                query = cmd[7:]
                results = p9.search_command(query)
                for r in results:
                    print(f"  {r}")
            elif cmd == "info":
                info = p9.system_info()
                for k, v in info.items():
                    print(f"{k}: {v}")
            elif cmd == "compile":
                print(p9.compile_vmx())
            else:
                output = p9.execute(cmd)
                print(output)
                
        except KeyboardInterrupt:
            print("\nUse 'exit' to quit")
        except Exception as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        # Direct command execution
        p9 = Plan9Remote()
        cmd = " ".join(sys.argv[1:])
        print(p9.execute(cmd))
    else:
        # Interactive mode
        interactive_shell()