# Lux9 System Diagrams

## 1. Pebble Memory Management Lifecycle

```mermaid
stateDiagram-v2
    state "Global Colorless Bank" as GlobalBank
    state "Process Colorless Bank" as Colorless
    state "White Token (Reservation)" as White
    state "Black Token (Allocated)" as Black
    state "Blue Token (I/O Borrow)" as Blue
    state "Red Token (Snapshot)" as Red

    GlobalBank --> Colorless: pebble_increase_budget (PoW)
    Colorless --> White: pebble_issue_white
    White --> Black: pebble_black_alloc (Verify & Mint Cap)
    Colorless --> Blue: pebble_blue_alloc (Independent)
    Colorless --> Red: pebble_red_alloc (Independent)
    Blue --> Red: pebble_red_snapshot (Copy Data)
    
    Black --> Colorless: pebble_black_free (Burn Cap)
    Blue --> Colorless: pebble_blue_free
    Red --> Colorless: pebble_red_free
```

## 2. 9P Router & Family HAL Flow

```mermaid
sequenceDiagram
    participant Process
    participant 9PRouter as 9P Router (kernel/9p_router.c)
    participant FamilyReg as Family Registry
    participant PCI as PCI Family (kernel/family/pci.c)
    participant Device as Hardware Device

    Process->>9PRouter: Twrite(fid, data)
    Note right of Process: syscall(SYS_WRITE)
    9PRouter->>9PRouter: Resolve FID to Channel
    9PRouter->>FamilyReg: family_lookup(type)
    9PRouter->>PCI: family->ops->write(chan, data)
    PCI->>Device: MMIO Write / DMA
    Device-->>PCI: Interrupt / Completion
    PCI-->>9PRouter: Return bytes written
    9PRouter-->>Process: Rwrite(count)
```

## 3. Security Architecture (Kinetic Defense & TPM)

```mermaid
graph TD
    subgraph "Kinetic Defense (pow_gate.c)"
        Req[Resource Request] --> Calc{Calculate Difficulty}
        Calc -->|Load + Magnitude| Diff[Target Zeros]
        Req --> Hash[SipHash(Seed + Nonce + Context)]
        Hash --> Verify{Leading Zeros >= Target?}
        Verify -->|Yes| Permit[Allow Operation]
        Verify -->|No| Deny[Block Operation]
    end

    subgraph "Hardware Root of Trust"
        TPM[TPM 2.0 Hardware]
        Driver[TPM Driver (tpm2_driver.c)]
        SAPI[Minimal SAPI (tpm2_sapi_minimal.c)]
        
        TPM <--> Driver
        Driver <--> SAPI
    end

    subgraph "Cryptographic Services"
        SAPI --> Random[Hardware RNG]
        SAPI --> Seal[Data Sealing (PCRs)]
        Permit --> Pebble[Pebble Memory Alloc]
        Pebble --> BlindLedger[Blind Ledger Capabilities]
        BlindLedger --> Mono[Monocypher (Crypto Primitives)]
    end
```
