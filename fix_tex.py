import re

keys = [
    "Complexity", "Attack Surface", "Performance Overhead",
    "Token scarcity", "Process behavior", "Decentralized verification",
    "Principle of least privilege", "Transferable", "Grace Period",
    "Memory Reuse Safety", "Consistency Guarantees", "Transfer Semantics",
    "Revocation Safety", "Chain of Trust", "Depth Limitation",
    "Platform Attestation", "Boot-Time Initialization", "Integrity Verification",
    "Incremental Updates", "Unified Security Model", "Memory Efficiency",
    "Derivation Chains", "Future Implications", "Precision and Granularity",
    "Performance Considerations", "Operational Excellence",
    "Proactive vs Reactive", "Software vs Hardware",
    "Economic vs Administrative", "Capability-based vs ACL",
    "Production"
]

with open("lux9_technical_manual.tex", "r") as f:
    content = f.read()

# Pattern: \textbf{KEY : -> \textbf{KEY}:
# Because we previously replaced "**:" with " :", we likely have "\textbf{KEY :"
# We need to ensure we don't double-brace if brace exists.
# But we assume brace is missing based on "Runaway argument".

for key in keys:
    # Escape key for regex
    esc_key = re.escape(key)
    # Search for \textbf{KEY : (assuming space from previous sed)
    # Or \textbf{KEY: if sed stripped space
    # Previous sed was s/\*\*:/ :/g.
    # So "Key**:" -> "Key :".
    # So "\textbf{Key**:" -> "\textbf{Key :".
    
    pattern = r"\textbf\{" + esc_key + r"\s*:"
    replacement = r"\textbf{" + key + r"}:"
    
    content = re.sub(pattern, replacement, content)

with open("lux9_technical_manual.tex", "w") as f:
    f.write(content)
