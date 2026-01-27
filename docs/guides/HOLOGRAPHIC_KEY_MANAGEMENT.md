# Holographic Key Management
## "The Invisible Lock"

**Concept:** Combine **Pebble Holographic Channels** (3-bit pointer tagging) with **Elligator 2** (steganographic keys) to create a "Secret Lock" mechanism.

### The Problem
Even with encrypted vaults, the *presence* of a lock is visible metadata. An attacker knows "this memory is locked."

### The Solution: Holographic Signaling
We use the 3-bit alignment gap ("The Peg") in pointers to signal hidden states to the kernel.

### Wave Definitions

| Wave (Bits 0-2) | Usage | Meaning |
| :--- | :--- | :--- |
| **0 (`000`)** | **Standard** | Normal pointer. Data is plaintext or standard ciphertext. |
| **1-3** | **Priority** | QoS tagging for MsgOrd (1=Low, 2=Med, 3=High). |
| **4** | **Reserved** | Future expansion. |
| **5** | **Reserved** | Future expansion. |
| **6 (`110`)** | **SECRET LOCK** | **"The Invisible Lock"**<br>The memory at this address is NOT random noise. It is an **Elligator 2** mapped public key.<br>The kernel must use `crypto_elligator_rev` to recover the key and unlock the vault.<br>To an observer, it looks like uninitialized/garbage RAM. |
| **7 (`111`)** | **DISTRESS** | **OOB Signal / Poison Pill**<br>Immediate termination or special handling required. Do not dereference payload. |

### Workflow: The Wave 6 Lock

1.  **Locking (Sleep):**
    *   SIP Process finishes work.
    *   Kernel encrypts Vault with ephemeral key $K$.
    *   Kernel uses `crypto_elligator_map(K)` to generate a random-looking string $R$.
    *   Kernel writes $R$ to the Vault header.
    *   **Crucial:** The kernel returns a pointer to the process tagged with **Wave 6** (`ptr | 0x6`).

2.  **Sleeping State:**
    *   Memory contains $R$ (looks like noise).
    *   Process holds `ptr` (value implies Wave 6).
    *   Attacker scanning RAM sees garbage.

3.  **Unlocking (Wake):**
    *   Process passes `ptr` back to Kernel (e.g., via `9p_dispatch`).
    *   Kernel detects **Wave 6**.
    *   Kernel reads $R$, applies `crypto_elligator_rev(R)` to get $K$.
    *   Kernel uses $K$ to decrypt the Vault.
    *   Vault is now open.

### Security Properties
*   **Plausible Deniability:** "That's not a key, that's just uninitialized memory."
*   **Steganography:** The "Lock" flag is hidden in the pointer alignment, not a struct field.
*   **Performance:** Check is bitwise (`ptr & 0x7`). Conversion is constant-time Elligator.
