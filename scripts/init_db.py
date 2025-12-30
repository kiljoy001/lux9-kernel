#!/usr/bin/env python3
"""
Init Binary Manifest Database
Tracks all init binaries built for the system with cryptographic verification.

Security Model:
- Init binaries are signed with Ed25519 keys
- Multiple signatures can be added (for key rotation)
- Production builds require VERIFIED status
- Development builds can use UNSIGNED (with warning)
"""

import sqlite3
import hashlib
import os
import sys
import subprocess
import base64
from datetime import datetime
from pathlib import Path

DB_PATH = Path(__file__).parent.parent / "build" / "init_manifest.sqlite"
CANONICAL_INIT_DIR = Path(__file__).parent.parent / "build"
KEYS_DIR = Path(__file__).parent.parent / "build" / "signing_keys"
REPO_ROOT = Path(__file__).parent.parent

# Old signing system integration
HOST_SIGN = REPO_ROOT / "userspace" / "host_sign"
HOST_SIGN_SRC = REPO_ROOT / "userspace" / "host_sign.c"
MONOCYPHER_C = REPO_ROOT / "kernel" / "crypto" / "monocypher.c"
MONOCYPHER_H = REPO_ROOT / "kernel" / "crypto" / "monocypher.h"

def ensure_host_sign_built():
    """Build host_sign binary if needed"""
    if HOST_SIGN.exists():
        return True

    print(f"Building {HOST_SIGN}...")

    # Check dependencies
    if not MONOCYPHER_C.exists():
        print(f"❌ Error: {MONOCYPHER_C} not found", file=sys.stderr)
        return False

    if not MONOCYPHER_H.exists():
        print(f"❌ Error: {MONOCYPHER_H} not found", file=sys.stderr)
        return False

    # Copy monocypher to userspace for #include
    userspace_dir = REPO_ROOT / "userspace"
    subprocess.run(['cp', MONOCYPHER_C, userspace_dir / "monocypher.c"], check=True)
    subprocess.run(['cp', MONOCYPHER_H, userspace_dir / "monocypher.h"], check=True)

    # Compile host_sign
    try:
        subprocess.run(
            ['gcc', '-o', HOST_SIGN, HOST_SIGN_SRC],
            check=True,
            cwd=REPO_ROOT
        )
        print(f"✅ Built {HOST_SIGN}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to compile host_sign: {e}", file=sys.stderr)
        return False

def get_tpm_key():
    """Attempt to unseal Ed25519 key from TPM (handle 0x81010001)"""
    try:
        # Check if tpm2_unseal is available
        subprocess.run(
            ['which', 'tpm2_unseal'],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )

        print("🔐 Attempting to unseal key from TPM (handle 0x81010001)...")
        # Unseal returns raw binary, convert to hex for host_sign
        key_bin = subprocess.check_output(['tpm2_unseal', '-c', '0x81010001'])
        key_hex = key_bin.hex()
        print(f"✅ Unsealed {len(key_bin)} byte key from TPM")
        return key_hex
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None

def get_signing_key():
    """Get signing key from environment variable or TPM"""
    # Try environment variable first
    key = os.environ.get("LUX_SIGNING_KEY")
    if key:
        print("🔑 Using signing key from LUX_SIGNING_KEY environment variable")
        return key

    # Try TPM
    key = get_tpm_key()
    if key:
        return key

    # No key found
    print("❌ Error: No signing key found", file=sys.stderr)
    print("   Methods tried:", file=sys.stderr)
    print("   1. Environment variable: LUX_SIGNING_KEY", file=sys.stderr)
    print("   2. TPM unsealing: handle 0x81010001", file=sys.stderr)
    print("", file=sys.stderr)
    print("   To generate a key:", file=sys.stderr)
    print("   python3 -c \"import secrets; print(secrets.token_hex(32))\"", file=sys.stderr)
    return None

def init_db():
    """Initialize the database schema"""
    DB_PATH.parent.mkdir(parents=True, exist_ok=True)

    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    c.execute('''
        CREATE TABLE IF NOT EXISTS init_binaries (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            sha256 TEXT NOT NULL UNIQUE,
            size INTEGER NOT NULL,
            canonical_path TEXT NOT NULL,
            signature_status TEXT NOT NULL CHECK(signature_status IN ('UNSIGNED', 'SIGNED', 'VERIFIED')),
            entry_point TEXT NOT NULL,
            source_path TEXT,
            build_commit TEXT,
            notes TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ''')

    c.execute('''
        CREATE TABLE IF NOT EXISTS signatures (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            init_binary_id INTEGER NOT NULL,
            signature_hex TEXT NOT NULL,
            public_key_hex TEXT,
            signed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            sig_file_path TEXT,
            FOREIGN KEY (init_binary_id) REFERENCES init_binaries(id)
        )
    ''')

    c.execute('''
        CREATE INDEX IF NOT EXISTS idx_sha256 ON init_binaries(sha256)
    ''')

    c.execute('''
        CREATE INDEX IF NOT EXISTS idx_timestamp ON init_binaries(timestamp DESC)
    ''')

    c.execute('''
        CREATE INDEX IF NOT EXISTS idx_signatures_binary ON signatures(init_binary_id)
    ''')

    conn.commit()
    conn.close()
    print(f"✅ Database initialized: {DB_PATH}")

def sha256_file(path):
    """Calculate SHA256 hash of a file"""
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        while chunk := f.read(8192):
            h.update(chunk)
    return h.hexdigest()

def get_entry_point(binary_path):
    """Extract ELF entry point using readelf"""
    try:
        result = subprocess.run(
            ['readelf', '-h', binary_path],
            capture_output=True,
            text=True,
            check=True
        )
        for line in result.stdout.splitlines():
            if 'Entry point' in line:
                return line.split()[-1]
    except subprocess.CalledProcessError:
        return "UNKNOWN"
    return "UNKNOWN"

def get_git_commit():
    """Get current git commit hash"""
    try:
        result = subprocess.run(
            ['git', 'rev-parse', 'HEAD'],
            capture_output=True,
            text=True,
            check=True,
            cwd=Path(__file__).parent.parent
        )
        return result.stdout.strip()[:12]
    except subprocess.CalledProcessError:
        return None

def register_init(binary_path, notes=None):
    """Register a new init binary in the database"""
    if not Path(binary_path).exists():
        print(f"❌ Error: Binary not found: {binary_path}", file=sys.stderr)
        return False

    # Calculate metadata
    sha256 = sha256_file(binary_path)
    size = Path(binary_path).stat().st_size
    entry_point = get_entry_point(binary_path)
    timestamp = int(datetime.now().timestamp())
    commit = get_git_commit()

    # Copy to canonical location
    canonical_path = CANONICAL_INIT_DIR / "init"
    canonical_path.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['cp', binary_path, canonical_path], check=True)
    subprocess.run(['chmod', '755', canonical_path], check=True)

    # Register in database
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    # Check if already registered
    c.execute('SELECT id, canonical_path FROM init_binaries WHERE sha256 = ?', (sha256,))
    existing = c.fetchone()
    if existing:
        print(f"ℹ️  Init binary already registered (ID: {existing[0]}, hash: {sha256[:16]}...)")
        print(f"   Path: {existing[1]}")
        conn.close()
        return True

    # Insert new record
    try:
        c.execute('''
            INSERT INTO init_binaries
            (timestamp, sha256, size, canonical_path, signature_status, entry_point, source_path, build_commit, notes)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (timestamp, sha256, size, str(canonical_path), 'UNSIGNED', entry_point, binary_path, commit, notes))

        conn.commit()
        new_id = c.lastrowid

        print(f"✅ Registered init binary (ID: {new_id}):")
        print(f"   Timestamp: {datetime.fromtimestamp(timestamp)}")
        print(f"   SHA256: {sha256}")
        print(f"   Size: {size} bytes")
        print(f"   Path: {canonical_path}")
        print(f"   Entry: {entry_point}")
        print(f"   Commit: {commit or 'N/A'}")
        print(f"   Status: UNSIGNED")

        conn.close()
        return True

    except sqlite3.IntegrityError as e:
        print(f"❌ Error: {e}", file=sys.stderr)
        conn.close()
        return False

def verify_init(binary_path):
    """Verify an init binary matches the database"""
    if not Path(binary_path).exists():
        print(f"❌ Error: Binary not found: {binary_path}", file=sys.stderr)
        return False

    sha256 = sha256_file(binary_path)
    size = Path(binary_path).stat().st_size

    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    c.execute('''
        SELECT id, timestamp, size, canonical_path, signature_status, entry_point, build_commit
        FROM init_binaries WHERE sha256 = ?
    ''', (sha256,))

    row = c.fetchone()
    conn.close()

    if not row:
        print(f"❌ VERIFICATION FAILED: Init binary NOT in database")
        print(f"   Hash: {sha256}")
        print(f"   Hint: Run 'init_db.py register <binary>' to add it")
        return False

    id, timestamp, db_size, path, sig_status, entry_point, commit = row

    print(f"✅ VERIFICATION PASSED (ID: {id})")
    print(f"   Registered: {datetime.fromtimestamp(timestamp)}")
    print(f"   SHA256: {sha256}")
    print(f"   Size: {size} bytes")
    print(f"   Signature: {sig_status}")
    print(f"   Entry: {entry_point}")
    print(f"   Commit: {commit or 'N/A'}")

    if size != db_size:
        print(f"⚠️  WARNING: Size mismatch (current: {size}, db: {db_size})")
        return False

    return True

def get_latest_init():
    """Get the path to the latest init binary"""
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    c.execute('''
        SELECT canonical_path, sha256 FROM init_binaries
        ORDER BY timestamp DESC LIMIT 1
    ''')

    row = c.fetchone()
    conn.close()

    if not row:
        print("❌ Error: No init binaries registered", file=sys.stderr)
        return None

    path, expected_sha256 = row

    if not Path(path).exists():
        print(f"❌ Error: Database references non-existent file: {path}", file=sys.stderr)
        return None

    # Verify hash
    current_sha256 = sha256_file(path)
    if current_sha256 != expected_sha256:
        print(f"❌ Error: Init binary corrupted! Hash mismatch:", file=sys.stderr)
        print(f"  Expected: {expected_sha256}", file=sys.stderr)
        print(f"  Current:  {current_sha256}", file=sys.stderr)
        return None

    return path

def sign_init(binary_id):
    """Sign an init binary using host_sign.c (Monocypher Ed25519)"""
    if not ensure_host_sign_built():
        return False

    # Get signing key
    signing_key = get_signing_key()
    if not signing_key:
        return False

    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    # Get binary info
    c.execute('SELECT canonical_path, sha256 FROM init_binaries WHERE id = ?', (binary_id,))
    row = c.fetchone()
    if not row:
        print(f"❌ Error: Init binary ID {binary_id} not found", file=sys.stderr)
        conn.close()
        return False

    canonical_path, sha256 = row

    if not Path(canonical_path).exists():
        print(f"❌ Error: Init binary not found: {canonical_path}", file=sys.stderr)
        conn.close()
        return False

    # Call host_sign to create .sig file
    print(f"🔏 Signing init binary ID {binary_id}...")
    try:
        result = subprocess.run(
            [HOST_SIGN, signing_key, canonical_path],
            capture_output=True,
            text=True,
            check=True
        )

        # Parse public key from host_sign output
        public_key_hex = None
        for line in result.stdout.splitlines():
            if line.startswith("Public Key:"):
                public_key_hex = line.split(": ")[1]
                break

        # Read the .sig file
        sig_path = Path(canonical_path).with_suffix(Path(canonical_path).suffix + '.sig')
        if not sig_path.exists():
            print(f"❌ Error: Signature file not created: {sig_path}", file=sys.stderr)
            conn.close()
            return False

        signature_bytes = sig_path.read_bytes()
        signature_hex = signature_bytes.hex()

        # Store signature in database
        c.execute('''
            INSERT INTO signatures (init_binary_id, signature_hex, public_key_hex, sig_file_path)
            VALUES (?, ?, ?, ?)
        ''', (binary_id, signature_hex, public_key_hex, str(sig_path)))

        # Update binary status to SIGNED
        c.execute('''
            UPDATE init_binaries SET signature_status = 'SIGNED'
            WHERE id = ?
        ''', (binary_id,))

        conn.commit()
        conn.close()

        print(f"✅ Signed init binary ID {binary_id}")
        print(f"   SHA256: {sha256}")
        print(f"   Signature file: {sig_path}")
        if public_key_hex:
            print(f"   Public key: {public_key_hex}")

        return True

    except subprocess.CalledProcessError as e:
        print(f"❌ Error signing binary: {e}", file=sys.stderr)
        print(f"   stdout: {e.stdout}", file=sys.stderr)
        print(f"   stderr: {e.stderr}", file=sys.stderr)
        conn.close()
        return False

def list_inits():
    """List all registered init binaries"""
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    c.execute('''
        SELECT id, timestamp, sha256, size, signature_status, entry_point, build_commit
        FROM init_binaries ORDER BY timestamp DESC
    ''')

    rows = c.fetchall()
    conn.close()

    if not rows:
        print("No init binaries registered")
        return

    print(f"{'ID':<5} {'Timestamp':<20} {'SHA256':<18} {'Size':<10} {'Status':<12} {'Entry':<12} {'Commit':<12}")
    print("-" * 100)

    for id, ts, sha256, size, status, entry, commit in rows:
        dt = datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
        print(f"{id:<5} {dt:<20} {sha256[:16]}... {size:<10} {status:<12} {entry:<12} {commit or 'N/A':<12}")

def main():
    if len(sys.argv) < 2:
        print("Usage:")
        print("  init_db.py init                         # Initialize database")
        print("  init_db.py register <binary> [notes]    # Register new init binary")
        print("  init_db.py verify <binary>              # Verify init binary")
        print("  init_db.py latest                       # Get latest init path")
        print("  init_db.py list                         # List all registered inits")
        print("")
        print("Signing commands (uses TPM or LUX_SIGNING_KEY env var):")
        print("  init_db.py sign <binary_id>             # Sign init binary with Ed25519")
        print("")
        print("Key sources (in priority order):")
        print("  1. LUX_SIGNING_KEY environment variable (64-char hex)")
        print("  2. TPM unsealing from handle 0x81010001")
        sys.exit(1)

    cmd = sys.argv[1]

    if cmd == 'init':
        init_db()
    elif cmd == 'register':
        if len(sys.argv) < 3:
            print("Error: Binary path required", file=sys.stderr)
            sys.exit(1)
        notes = sys.argv[3] if len(sys.argv) > 3 else None
        init_db()  # Ensure DB exists
        if not register_init(sys.argv[2], notes):
            sys.exit(1)
    elif cmd == 'verify':
        if len(sys.argv) < 3:
            print("Error: Binary path required", file=sys.stderr)
            sys.exit(1)
        if not verify_init(sys.argv[2]):
            sys.exit(1)
    elif cmd == 'latest':
        path = get_latest_init()
        if path:
            print(path)
        else:
            sys.exit(1)
    elif cmd == 'list':
        list_inits()
    elif cmd == 'sign':
        if len(sys.argv) < 3:
            print("Error: Binary ID required", file=sys.stderr)
            sys.exit(1)
        try:
            binary_id = int(sys.argv[2])
        except ValueError:
            print("Error: Binary ID must be an integer", file=sys.stderr)
            sys.exit(1)
        if not sign_init(binary_id):
            sys.exit(1)
    else:
        print(f"Error: Unknown command: {cmd}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()
