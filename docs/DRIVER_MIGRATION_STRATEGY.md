# Lux9 Driver Migration Strategy (Monolith to Microkernel)

To achieve seL4-level verification for the Lux9 kernel, we must migrate ~125k lines of legacy drivers (the "9front-port") to isolated WASM components.

## 1. The Kernel "Diverter" Stub (HAL Client)
The monolithic code (e.g., `devsd.c`) is replaced by a **9P Diverter** that communicates with the **Userspace HAL Server**.
- **Role**: Maintains a mapping between device paths and HAL Channel IDs.
- **Complexity**: ~200 lines of verified C (compared to 10k+ for the full driver).
- **Security**: The kernel only verifies that the caller has the capability to talk to the diverted HAL service.

## 2. The HAL Server & WASM Isolation
The original driver logic is compiled into a WASM module hosted by the **existing HAL server** (`userspace/hal`).
- **Isolation**: Runs in a sandboxed WASM memory space within the HAL process.
- **Hardware Access**: The HAL server uses the `FamilyExchangePage` and `Pebble` tokens to grant restricted hardware access to the WASM driver.

## 3. Case Study: `devsd` Migration to HAL
1. **Identify Logic**: `devsd.c` (Path parsing) and `devsd_hw.c` (AHCI/IDE I/O) are packaged as a HAL "Device Family".
2. **Implement Diverter**: Kernel's `/dev/sd` becomes a virtual 9P mount that routes requests to the HAL server's PCI family.
3. **Verify Contract**: The only thing we need to verify in the kernel is the **Routing Logic** to the HAL server.

---

### Phase 2: Action Items
- [ ] Create `kernel/9front-port/devdivert.c` (The Generic Diverter).
- [ ] Implement `CAP_HW_IO` check in the WASM Host Functions.
- [ ] Port `devsd_hw.c` logic to a WASM module using the new HAL interface.
