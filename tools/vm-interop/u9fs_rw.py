#!/usr/bin/env python3
"""
U9fs server with read-write access
Uses local Unix permissions (runs as your user)
"""
import socket
import subprocess
import sys
import os

PORT = 9564
ROOT_DIR = "/home/scott/Repo/VM-Interop"

def handle_connection(conn, addr):
    print(f"Connection from {addr}")
    
    pid = os.fork()
    if pid == 0:
        # Child process
        os.dup2(conn.fileno(), 0)  # stdin
        os.dup2(conn.fileno(), 1)  # stdout
        conn.close()
        
        # Run u9fs with -n (no network check) and -u (fixed user)
        # This allows read-write as the specified user
        os.execl("/usr/local/bin/u9fs", "u9fs", 
                 "-n",           # No network address check
                 "-a", "none",   # No auth (trusts the -u flag)
                 "-u", "scott",  # Run as this Unix user
                 "-D",           # Debug mode (optional)
                 "-l", "/tmp/u9fs.log",  # Log file
                 ROOT_DIR)
        
        sys.exit(1)
    else:
        conn.close()
        print(f"Spawned u9fs (pid {pid})")

def main():
    print(f"Starting u9fs R/W server on port {PORT}")
    print(f"Serving: {ROOT_DIR}")
    print("\nFrom 9front:")
    print(f"  srv tcp!10.0.2.2!{PORT} interop")
    print(f"  mount /srv/interop /n/interop")
    print("\nThis server runs as Unix user 'scott' with full R/W access")
    
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', PORT))
    server.listen(5)
    
    try:
        while True:
            conn, addr = server.accept()
            handle_connection(conn, addr)
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        server.close()

if __name__ == "__main__":
    # Make sure directory exists and is writable
    os.makedirs(ROOT_DIR, exist_ok=True)
    test_file = os.path.join(ROOT_DIR, ".write_test")
    try:
        with open(test_file, 'w') as f:
            f.write("test")
        os.remove(test_file)
        print(f"✓ Directory {ROOT_DIR} is writable")
    except Exception as e:
        print(f"✗ Cannot write to {ROOT_DIR}: {e}")
        sys.exit(1)
    
    main()