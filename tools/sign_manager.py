import sqlite3
import os
import hashlib
import subprocess
import sys
import time

DB_PATH = "initrd_signatures.db"
INITRD_ROOT = "userspace/build/init" # Updated to match Makefile output
HOST_SIGN = "userspace/host_sign"
HOST_SIGN_SRC = "userspace/host_sign.c"

def init_db():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS files
                 (path TEXT PRIMARY KEY, mtime REAL, size INTEGER, sha256 TEXT, signature BLOB, sign_ts REAL)''')
    conn.commit()
    return conn

def calculate_sha256(filepath):
    sha256_hash = hashlib.sha256()
    with open(filepath, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def ensure_signer_built():
    if not os.path.exists(HOST_SIGN):
        print(f"Building {HOST_SIGN}...")
        # Monocypher path needs to be handled. Assuming simple compilation for now.
        # We need to find monocypher.c. It's usually in kernel/crypto/
        monocypher_src = "kernel/crypto/monocypher.c"
        if not os.path.exists(monocypher_src):
             monocypher_src = "monocypher-4.0.2/src/monocypher.c" # Fallback

        if not os.path.exists(monocypher_src):
             print(f"Error: monocypher.c not found.")
             sys.exit(1)
             
        # Copy monocypher.c/h to userspace for the include to work or -I
        # host_sign.c does #include "monocypher.c", so we copy it to userspace/
        subprocess.check_call(["cp", monocypher_src, "userspace/monocypher.c"])
        
        # Header might be in a different place
        monocypher_hdr = monocypher_src.replace(".c", ".h")
        if not os.path.exists(monocypher_hdr):
             # Try common locations
             if os.path.exists("kernel/include/monocypher.h"):
                 monocypher_hdr = "kernel/include/monocypher.h"
             elif os.path.exists("monocypher-4.0.2/src/monocypher.h"):
                 monocypher_hdr = "monocypher-4.0.2/src/monocypher.h"
        
        if os.path.exists(monocypher_hdr):
             subprocess.check_call(["cp", monocypher_hdr, "userspace/monocypher.h"])
        else:
             print(f"Warning: monocypher.h not found, compilation might fail.")

        try:
            subprocess.check_call(["gcc", "-o", HOST_SIGN, HOST_SIGN_SRC])
        except subprocess.CalledProcessError as e:
            print(f"Failed to compile signer: {e}")
            sys.exit(1)

def sign_file(filepath, secret_key):
    print(f"Signing {filepath}...")
    try:
        # host_sign <key> <file> -> creates <file>.sig
        subprocess.check_call([HOST_SIGN, secret_key, filepath])
        
        sig_path = filepath + ".sig"
        if os.path.exists(sig_path):
            with open(sig_path, "rb") as f:
                return f.read()
    except Exception as e:
        print(f"Error signing {filepath}: {e}")
        return None
    return None

def process_file(conn, filepath, secret_key):
    rel_path = os.path.relpath(filepath, INITRD_ROOT)
    stat = os.stat(filepath)
    
    # Check DB
    c = conn.cursor()
    c.execute("SELECT mtime, size, sha256, signature FROM files WHERE path=?", (rel_path,))
    row = c.fetchone()
    
    current_hash = calculate_sha256(filepath)
    
    needs_signing = True
    signature = None
    
    if row:
        db_mtime, db_size, db_sha256, db_signature = row
        if db_size == stat.st_size and db_sha256 == current_hash:
            # File unchanged, reuse signature
            # print(f"Using cached signature for {rel_path}")
            signature = db_signature
            needs_signing = False
            
            # Ensure .sig file exists on disk
            sig_path = filepath + ".sig"
            if not os.path.exists(sig_path):
                with open(sig_path, "wb") as f:
                    f.write(signature)
        # else:
            # print(f"File changed: {rel_path}")
    
    if needs_signing:
        signature = sign_file(filepath, secret_key)
        if signature:
            c.execute("INSERT OR REPLACE INTO files (path, mtime, size, sha256, signature, sign_ts) VALUES (?, ?, ?, ?, ?, ?)",
                      (rel_path, stat.st_mtime, stat.st_size, current_hash, signature, time.time()))
            conn.commit()

def get_tpm_key():
    """Attempt to unseal key from TPM persistent handle 0x81010001"""
    try:
        # Check if tpm2_unseal is available
        subprocess.check_call(["which", "tpm2_unseal"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        
        print("Attempting to unseal key from TPM (handle 0x81010001)...")
        # Unseal returns raw binary, we need hex for host_sign
        key_bin = subprocess.check_output(["tpm2_unseal", "-c", "0x81010001"])
        return key_bin.hex()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None

def main():
    secret_key = os.environ.get("LUX_SIGNING_KEY")
    
    if not secret_key:
        secret_key = get_tpm_key()
        
    if not secret_key:
        print("Error: No signing key found.")
        print("Methods tried:")
        print("1. Environment variable LUX_SIGNING_KEY")
        print("2. TPM Unseal (handle 0x81010001)")
        sys.exit(1)
        
    ensure_signer_built()
        
    conn = init_db()
    
    if not os.path.exists(INITRD_ROOT):
         print(f"Initrd root {INITRD_ROOT} does not exist. Run 'make iso' or 'make userspace/build/initrd.tar' first.")
         sys.exit(1)

    count = 0
    for root, dirs, files in os.walk(INITRD_ROOT):
        for name in files:
            if name.endswith(".sig"):
                continue
            filepath = os.path.join(root, name)
            # Sign everything in the initrd
            process_file(conn, filepath, secret_key)
            count += 1
                 
    conn.close()
    print(f"Signature check complete for {count} files.")

if __name__ == "__main__":
    main()
