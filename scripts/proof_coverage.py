#!/usr/bin/env python3
"""
Coq Proof Coverage Mapper

Maps Coq proof files in proofs/ to their corresponding kernel C files.
Generates coverage reports showing what IS and ISN'T formally proven.

Usage:
  python3 scripts/proof_coverage.py          # Generate coverage report
  python3 scripts/proof_coverage.py --json   # Output as JSON
  python3 scripts/proof_coverage.py --md     # Output as Markdown
"""

import os
import re
import json
import sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Set, Optional

# Configuration
REPO_ROOT = Path(__file__).parent.parent
PROOFS_DIR = REPO_ROOT / "proofs"
KERNEL_DIR = REPO_ROOT / "kernel"

@dataclass
class ProofFile:
    """Represents a single Coq proof file"""
    path: Path
    theorems: List[str] = field(default_factory=list)
    lemmas: List[str] = field(default_factory=list)
    admits: List[str] = field(default_factory=list)
    target_files: List[str] = field(default_factory=list)  # C files this proves
    
    @property
    def is_complete(self) -> bool:
        return len(self.admits) == 0 and (len(self.theorems) + len(self.lemmas)) > 0
    
    @property
    def total_proofs(self) -> int:
        return len(self.theorems) + len(self.lemmas)
    
    @property
    def completion_pct(self) -> int:
        total = self.total_proofs
        if total == 0:
            return 0
        proven = total - len(self.admits)
        return int(proven * 100 / total)

@dataclass  
class KernelFile:
    """Represents a kernel C file"""
    path: Path
    has_acsl: bool = False
    proof_files: List[Path] = field(default_factory=list)
    
    @property
    def has_proofs(self) -> bool:
        return len(self.proof_files) > 0


# Mapping rules: proof directory -> kernel file patterns
# This is the key configuration that ties proofs to C files
PROOF_MAPPING = {
    "pebble": ["kernel/pebble.c", "kernel/pebble_kernel.c"],
    "blind_ledger": ["kernel/9front-port/blind_ledger.c"],
    "ramdisk": ["kernel/9front-port/devram.c"],
    "9p_router": ["kernel/9p_router.c"],
    "proc": ["kernel/9front-port/proc.c", "kernel/proc_fsm.c"],
    "scheduler": ["kernel/9front-port/edf.c"],
    "borrow": ["kernel/borrowchecker.c", "kernel/borrow_enforce.c"],
    "capability": ["kernel/capability/clr_capability.c"],
    "clr": [],  # CLR proofs are for the WASM/CIL runtime
    "msgord": ["kernel/msgord.c"],
    "relooper": [],  # Relooper proofs are for CIL compilation
    "mnt": ["kernel/9front-port/devmnt.c"],
    "cache": ["kernel/9front-port/cache.c"],
    "pipe": ["kernel/9front-port/devpipe.c"],
    "sip": ["kernel/9front-port/devsip.c"],
}


def parse_coq_file(path: Path) -> ProofFile:
    """Parse a Coq file to extract theorems, lemmas, and admits"""
    pf = ProofFile(path=path)
    
    try:
        content = path.read_text(errors='ignore')
        
        # Find theorems
        pf.theorems = re.findall(r'Theorem\s+(\w+)', content)
        
        # Find lemmas
        pf.lemmas = re.findall(r'Lemma\s+(\w+)', content)
        
        # Find admits (incomplete proofs)
        # Look for "Admitted." or "admit." tactic
        admits_in_file = len(re.findall(r'Admitted\.', content))
        if admits_in_file > 0:
            # Try to find which theorems are admitted
            # This is a heuristic - look for Theorem/Lemma followed by Admitted
            blocks = re.split(r'(?=Theorem|Lemma)', content)
            for block in blocks:
                if 'Admitted.' in block:
                    match = re.match(r'(Theorem|Lemma)\s+(\w+)', block)
                    if match:
                        pf.admits.append(match.group(2))
        
        # Check if file references specific C files
        # Look for comments like "(* Proves: kernel/pebble.c *)"
        refs = re.findall(r'\(\*\s*[Pp]roves?:?\s*([^\*]+)\*\)', content)
        for ref in refs:
            pf.target_files.extend([f.strip() for f in ref.split(',')])
            
    except Exception as e:
        print(f"Warning: Could not parse {path}: {e}", file=sys.stderr)
    
    return pf


def scan_proofs() -> Dict[str, List[ProofFile]]:
    """Scan all proof directories and parse Coq files"""
    proofs_by_dir: Dict[str, List[ProofFile]] = {}
    
    if not PROOFS_DIR.exists():
        return proofs_by_dir
    
    for subdir in PROOFS_DIR.iterdir():
        if subdir.is_dir() and not subdir.name.startswith('.'):
            proof_files = []
            for vfile in subdir.glob("*.v"):
                pf = parse_coq_file(vfile)
                proof_files.append(pf)
            if proof_files:
                proofs_by_dir[subdir.name] = proof_files
    
    return proofs_by_dir


def scan_kernel_files() -> Dict[str, KernelFile]:
    """Scan kernel directory for C files"""
    kernel_files: Dict[str, KernelFile] = {}
    
    for cfile in KERNEL_DIR.rglob("*.c"):
        rel_path = str(cfile.relative_to(REPO_ROOT))
        
        # Check for ACSL annotations
        has_acsl = False
        try:
            content = cfile.read_text(errors='ignore')
            has_acsl = '/*@' in content
        except:
            pass
        
        kernel_files[rel_path] = KernelFile(path=cfile, has_acsl=has_acsl)
    
    return kernel_files


def map_proofs_to_files(proofs: Dict[str, List[ProofFile]], 
                        kernel_files: Dict[str, KernelFile]) -> None:
    """Map proof directories to kernel files using PROOF_MAPPING"""
    
    for proof_dir, proof_list in proofs.items():
        # Get target C files from mapping
        targets = PROOF_MAPPING.get(proof_dir, [])
        
        for target in targets:
            if target in kernel_files:
                for pf in proof_list:
                    kernel_files[target].proof_files.append(pf.path)


def generate_report(proofs: Dict[str, List[ProofFile]], 
                   kernel_files: Dict[str, KernelFile],
                   format: str = "text") -> str:
    """Generate a coverage report"""
    
    # Calculate statistics
    total_kernel = len(kernel_files)
    with_proofs = sum(1 for kf in kernel_files.values() if kf.has_proofs)
    with_acsl = sum(1 for kf in kernel_files.values() if kf.has_acsl)
    
    total_theorems = sum(pf.total_proofs for pfs in proofs.values() for pf in pfs)
    total_admits = sum(len(pf.admits) for pfs in proofs.values() for pf in pfs)
    proven = total_theorems - total_admits
    
    if format == "json":
        return json.dumps({
            "kernel_files": {
                "total": total_kernel,
                "with_coq_proofs": with_proofs,
                "with_acsl": with_acsl,
                "proof_coverage_pct": int(with_proofs * 100 / total_kernel) if total_kernel else 0
            },
            "coq_proofs": {
                "total_theorems": total_theorems,
                "proven": proven,
                "admitted": total_admits,
                "completion_pct": int(proven * 100 / total_theorems) if total_theorems else 0
            },
            "mappings": {
                dir: [str(pf.path.relative_to(REPO_ROOT)) for pf in pfs]
                for dir, pfs in proofs.items()
            },
            "proven_files": [
                path for path, kf in kernel_files.items() if kf.has_proofs
            ],
            "unproven_files": [
                path for path, kf in kernel_files.items() if not kf.has_proofs
            ]
        }, indent=2)
    
    elif format == "md":
        lines = [
            "# Coq Proof Coverage Report",
            "",
            "## Summary",
            "",
            "| Metric | Value |",
            "|--------|-------|",
            f"| Total Kernel C Files | {total_kernel} |",
            f"| Files with Coq Proofs | {with_proofs} |",
            f"| Files with ACSL | {with_acsl} |",
            f"| **Proof Coverage** | **{int(with_proofs * 100 / total_kernel) if total_kernel else 0}%** |",
            "",
            "| Coq Metric | Value |",
            "|------------|-------|",
            f"| Total Theorems/Lemmas | {total_theorems} |",
            f"| Proven | {proven} |",
            f"| Admitted | {total_admits} |",
            f"| **Completion** | **{int(proven * 100 / total_theorems) if total_theorems else 0}%** |",
            "",
            "## Proven Kernel Files ✅",
            "",
        ]
        
        for path, kf in sorted(kernel_files.items()):
            if kf.has_proofs:
                proofs_str = ", ".join(str(p.name) for p in kf.proof_files)
                lines.append(f"- `{path}` ← {proofs_str}")
        
        lines.extend([
            "",
            "## Unproven Kernel Files ❌",
            "",
            "> [!WARNING]",
            "> These kernel files have NO formal Coq proofs:",
            "",
        ])
        
        # Group by directory for readability
        unproven = [path for path, kf in kernel_files.items() if not kf.has_proofs]
        dirs: Dict[str, List[str]] = {}
        for path in unproven:
            d = str(Path(path).parent)
            dirs.setdefault(d, []).append(Path(path).name)
        
        for d, files in sorted(dirs.items()):
            lines.append(f"**{d}/** ({len(files)} files)")
            for f in sorted(files)[:10]:  # Limit per dir
                lines.append(f"  - {f}")
            if len(files) > 10:
                lines.append(f"  - ... and {len(files) - 10} more")
            lines.append("")
        
        lines.extend([
            "## Proof Directory Mappings",
            "",
            "| Proof Directory | Proves |",
            "|-----------------|--------|",
        ])
        
        for dir_name, targets in sorted(PROOF_MAPPING.items()):
            if targets:
                lines.append(f"| `proofs/{dir_name}/` | {', '.join(f'`{t}`' for t in targets)} |")
            else:
                lines.append(f"| `proofs/{dir_name}/` | *(infrastructure proofs)* |")
        
        return "\n".join(lines)
    
    else:  # text format
        lines = [
            "═" * 60,
            "Coq Proof Coverage Report",
            "═" * 60,
            "",
            f"Kernel Files:     {total_kernel}",
            f"With Coq Proofs:  {with_proofs} ({int(with_proofs * 100 / total_kernel) if total_kernel else 0}%)",
            f"With ACSL:        {with_acsl}",
            "",
            f"Theorems/Lemmas:  {total_theorems}",
            f"Proven:           {proven}",
            f"Admitted:         {total_admits}",
            "",
            "─" * 60,
            "PROVEN FILES:",
            "─" * 60,
        ]
        
        for path, kf in sorted(kernel_files.items()):
            if kf.has_proofs:
                lines.append(f"  ✅ {path}")
        
        lines.extend([
            "",
            "─" * 60,
            "UNPROVEN FILES (no Coq proofs):",
            "─" * 60,
        ])
        
        for path, kf in sorted(kernel_files.items()):
            if not kf.has_proofs:
                lines.append(f"  ❌ {path}")
        
        return "\n".join(lines)


def main():
    format = "text"
    if "--json" in sys.argv:
        format = "json"
    elif "--md" in sys.argv:
        format = "md"
    
    # Scan proofs and kernel files
    proofs = scan_proofs()
    kernel_files = scan_kernel_files()
    
    # Map proofs to C files
    map_proofs_to_files(proofs, kernel_files)
    
    # Generate and print report
    report = generate_report(proofs, kernel_files, format)
    print(report)


if __name__ == "__main__":
    main()
