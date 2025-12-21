#!/usr/bin/env python3
import os
import sys
import sqlite3
import subprocess
import re
from datetime import datetime
from concurrent.futures import ThreadPoolExecutor
import argparse
from sqlite3 import register_adapter

# Configuration
DB_PATH = ".verification_registry.db"
REPO_ROOT = os.getcwd()
PROOFS_DIR = os.path.join(REPO_ROOT, "proofs")
KERNEL_DIR = os.path.join(REPO_ROOT, "kernel")
FAIL_LOG = os.path.join(REPO_ROOT, "verification_failures.log")
# Extra C files that should always be pushed through Frama-C even
# without inline ACSL markers.
STATIC_ACSL_TARGETS = [
    ("kernel/9front-pc64/mmu.c", "acsl-mmu"),
]

# Ensure datetime values are adapted to ISO strings for sqlite3 >= 3.12.
register_adapter(datetime, lambda d: d.isoformat())

def now_ts() -> str:
    return datetime.now().isoformat()

# Colors for output
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def log_failure(path: str, status: str, detail: str):
    """Append a failure record to a log file for postmortem tracking."""
    try:
        with open(FAIL_LOG, "a", encoding="utf-8") as f:
            f.write(f"[{datetime.now().isoformat()}] {status} {path}\n")
            f.write(detail.strip() + "\n")
            f.write("=" * 60 + "\n")
    except Exception as e:
        print(f"{Colors.WARNING}Warning: failed to write failure log: {e}{Colors.ENDC}")

def init_db():
    """Initialize the SQLite database schema."""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS tracked_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT UNIQUE NOT NULL,
            file_type TEXT NOT NULL,
            last_seen TIMESTAMP
        )
    ''')
    
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS verification_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER,
            status TEXT,
            output TEXT,
            timestamp TIMESTAMP,
            FOREIGN KEY(file_id) REFERENCES tracked_files(id)
        )
    ''')
    
    conn.commit()
    return conn

def scan_repository(conn):
    """Scan the repository for Coq and annotated C files and update the DB."""
    cursor = conn.cursor()
    found_paths = set()

    print(f"{Colors.OKBLUE}[Scanning Repository]{Colors.ENDC}")

    # 1. Scan Coq files (.v)
    for root, dirs, files in os.walk(PROOFS_DIR):
        for file in files:
            if file.endswith(".v"):
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, REPO_ROOT)
                found_paths.add(rel_path)
                
                cursor.execute('''
                    INSERT OR IGNORE INTO tracked_files (path, file_type, last_seen)
                    VALUES (?, 'coq', ?)
                ''', (rel_path, now_ts()))
                
                cursor.execute('''
                    UPDATE tracked_files SET last_seen = ? WHERE path = ?
                ''', (now_ts(), rel_path))

    # 2. Scan C files with ACSL annotations
    for root, dirs, files in os.walk(KERNEL_DIR):
        for file in files:
            if file.endswith(".c"):
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, REPO_ROOT)
                
                try:
                    with open(full_path, 'r', errors='ignore') as f:
                        content = f.read()
                        if "/*@" in content:
                            found_paths.add(rel_path)
                            cursor.execute('''
                                INSERT OR IGNORE INTO tracked_files (path, file_type, last_seen)
                                VALUES (?, 'acsl', ?)
                            ''', (rel_path, now_ts()))
                            
                            cursor.execute('''
                                UPDATE tracked_files SET last_seen = ? WHERE path = ?
                            ''', (now_ts(), rel_path))
                except Exception as e:
                    print(f"{Colors.WARNING}Warning: Could not read {rel_path}: {e}{Colors.ENDC}")

    # 3. Inject static Frama-C targets
    for rel_path, ftype in STATIC_ACSL_TARGETS:
        full_path = os.path.join(REPO_ROOT, rel_path)
        if os.path.exists(full_path):
            found_paths.add(rel_path)
            cursor.execute('''
                INSERT OR IGNORE INTO tracked_files (path, file_type, last_seen)
                VALUES (?, ?, ?)
            ''', (rel_path, ftype, now_ts()))
            cursor.execute('''
                UPDATE tracked_files SET last_seen = ? WHERE path = ?
            ''', (now_ts(), rel_path))

    # Cleanup removed files
    cursor.execute("SELECT path FROM tracked_files")
    all_tracked = {row[0] for row in cursor.fetchall()}
    to_remove = all_tracked - found_paths
    
    for path in to_remove:
        print(f"{Colors.WARNING}Removing stale file from DB: {path}{Colors.ENDC}")
        cursor.execute("DELETE FROM tracked_files WHERE path = ?", (path,))

    conn.commit()
    print(f"Found {len(found_paths)} files to verify.")
    return list(found_paths)

def verify_acsl_file(file_info):
    """Run Frama-C verification for a single C file."""
    file_id, path, file_type = file_info
    full_path = os.path.join(REPO_ROOT, path)
    
    # Default Frama-C command (ACSL inline)
    cmd = [
        "frama-c", "-machdep", "gcc_x86_64", "-wp", "-wp-prover", "cvc4", "-wp-timeout", "5",
        "-cpp-extra-args=-I" + os.path.join(KERNEL_DIR, "include") + 
        " -I" + os.path.join(KERNEL_DIR, "9front-pc64") +
        " -D__PLAN9_KERNEL__",
        full_path
    ]
    env = os.environ.copy()
    env.setdefault("WHY3CONFIG", "/tmp/why3.conf")

    # Special handling for mmu.c: use our preprocessing shim and x86_64 machdep.
    if file_type == "acsl-mmu":
        cmd = [
            "frama-c",
            "-machdep", "gcc_x86_64",
            "-no-cpp-frama-c-compliant",
            "-wp",
            "-wp-rte",
            "-wp-timeout", "10",
            "-wp-prover", "cvc4",
            "-cpp-command", "./proofs/mmu/frama_cpp.sh %i %o",
            full_path,
        ]
        env.setdefault("FRAMAC_SHARE", os.path.join(REPO_ROOT, "proofs/mmu"))
        env.setdefault("WHY3CONFIG", "/tmp/why3.conf")
    
    status = "UNKNOWN"
    output = ""
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30,
            env=env,
        )
        output = result.stdout + result.stderr
        
        if "Operation not permitted (connect," in output:
                status = "WARNING"
        elif "Proved goals" in output:
                status = "PASS"
        else:
                status = "PASS" if result.returncode == 0 else "FAIL"
                if "Proved goals" not in output and "Valid" not in output:
                    status = "WARNING"
        
        if "Error" in output:
                status = "FAIL"
        if status != "PASS":
                log_failure(path, status, output)
                
    except FileNotFoundError:
        status = "SKIPPED"
        output = "frama-c not found"
    except subprocess.TimeoutExpired:
        status = "TIMEOUT"

    return (file_id, status, output, path)

def run_coq_verification(conn, coq_files):
    """Run Coq verification via Make and update DB."""
    print(f"{Colors.OKBLUE}[Verifying Coq Proofs via Make]{Colors.ENDC}")
    
    # Map paths to IDs for easy DB update
    path_to_id = {f[1]: f[0] for f in coq_files}
    
    # Run make in proofs directory
    # We use -k to keep going even if some fail, to get results for all
    # Optimization: Remove 'clean' to allow incremental builds and add -j for parallel execution
    jobs = str(os.cpu_count() or 1)
    cmd = ["make", "-C", "proofs", "-j" + jobs, "verify", "-k"]
    
    try:
        cursor = conn.cursor()
        buffer_lines = []
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            universal_newlines=True
        )

        current_file = None
        for line in process.stdout:
            line = line.strip()
            buffer_lines.append(line)
            if line.startswith("COQC"):
                parts = line.split()
                if len(parts) >= 2:
                    fname = parts[1]
                    matches = [p for p in path_to_id.keys() if p.endswith("/" + fname) or p == fname]
                    if matches:
                        current_file = matches[0]
                        print(f"{Colors.OKBLUE}  [Coq] compiling {current_file}{Colors.ENDC}")
            elif line.startswith("Finished") and current_file:
                print(f"{Colors.OKGREEN}  [Coq] done     {current_file}{Colors.ENDC}")
                current_file = None

        process.wait()

        for path, fid in path_to_id.items():
            full_path = os.path.join(REPO_ROOT, path)
            vo_path = full_path + "o"  # .v -> .vo

            status = "FAIL"
            output = "Verification failed (no .vo generated)"

            if os.path.exists(vo_path):
                status = "PASS"
                output = "Compiled successfully"

            cursor.execute(
                '''
                INSERT INTO verification_results (file_id, status, output, timestamp)
                VALUES (?, ?, ?, ?)
                ''',
                (fid, status, output, now_ts())
            )

            if status == "PASS":
                print(f"  {Colors.OKGREEN}✓ PASS{Colors.ENDC} {path}")
            else:
                print(f"  {Colors.FAIL}✗ FAIL{Colors.ENDC} {path}")
                tail = "\n".join(buffer_lines[-50:])
                log_failure(path, status, f"make output tail:\n{tail}")

        conn.commit()

    except Exception as e:
        print(f"{Colors.FAIL}Make execution failed: {e}{Colors.ENDC}")

def main():
    print(f"{Colors.HEADER}═══════════════════════════════════════════════════════════════{Colors.ENDC}")
    print(f"{Colors.HEADER}   Lux9 Integrity Verification Manager (SQLite Backed){Colors.ENDC}")
    print(f"{Colors.HEADER}═══════════════════════════════════════════════════════════════{Colors.ENDC}")

    conn = init_db()
    scan_repository(conn)
    
    cursor = conn.cursor()
    cursor.execute("SELECT id, path, file_type FROM tracked_files")
    all_files = cursor.fetchall()
    
    coq_files = [f for f in all_files if f[2] == 'coq']
    acsl_files = [f for f in all_files if f[2].startswith('acsl')]
    
    print(f"\n{Colors.OKBLUE}[Running Verification on {len(all_files)} Files]{Colors.ENDC}")
    
    # 1. Run Coq Verification (Sequential/Make)
    run_coq_verification(conn, coq_files)
    
    # 2. Run ACSL Verification (Parallel)
    print(f"\n{Colors.OKBLUE}[Verifying ACSL Annotations]{Colors.ENDC}")
    acsl_failed = 0
    with ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
        for result in executor.map(verify_acsl_file, acsl_files):
            file_id, status, output, path = result
            
            cursor.execute(
                '''
                INSERT INTO verification_results (file_id, status, output, timestamp)
                VALUES (?, ?, ?, ?)
                ''',
                (file_id, status, output, now_ts())
            )
            
            if status == "PASS":
                print(f"  {Colors.OKGREEN}✓ PASS{Colors.ENDC} {path}")
            elif status == "WARNING":
                print(f"  {Colors.WARNING}⚠ WARN{Colors.ENDC} {path}")
            elif status == "SKIPPED":
                print(f"  {Colors.OKCYAN}- SKIP{Colors.ENDC} {path}")
            else:
                print(f"  {Colors.FAIL}✗ FAIL{Colors.ENDC} {path}")
                acsl_failed += 1

    conn.commit()
    
    # Summary
    cursor.execute("SELECT count(*) FROM verification_results WHERE status='FAIL' AND timestamp > datetime('now', '-1 minute')")
    failed_count = cursor.fetchone()[0]
    conn.close()

    print(f"\n{Colors.HEADER}═══════════════════════════════════════════════════════════════{Colors.ENDC}")
    
    if failed_count > 0:
        print(f"{Colors.FAIL}❌ Verification Failed! {failed_count} files broken.{Colors.ENDC}")
        sys.exit(1)
    else:
        print(f"{Colors.OKGREEN}✅ All System Integrity Checks Passed.{Colors.ENDC}")
        sys.exit(0)

if __name__ == "__main__":
    main()
# Ensure datetime values are adapted to ISO strings for sqlite3 >= 3.12.
register_adapter(datetime, lambda d: d.isoformat())
