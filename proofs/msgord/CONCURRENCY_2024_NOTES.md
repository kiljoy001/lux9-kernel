# MSGORD 2024 Concurrency Model - Proof Coverage

## Background

As of the 2024 changes (incorporated in kernel/msgord.c and kernel/include/msgord.h), 
MSGORD has a NEW concurrency model based on **explicit resource-key conflicts** rather than
implicit parent-chain analysis.

## Key Changes

1. **`MsgOrdSpec` is explicit** - callers provide resource keys that an operation conflicts on
2. **Anticone is "conflicting pending non-parents"** - NOT "all pending non-parents"
3. **Conflict detection** = shared resource key OR zero keys (global barrier)
4. **Parents selected from frontier tips**, not from tail

## What the Existing Proofs Cover

The existing Coq proofs in `proofs/msgord/` are COMPLETE and VERIFIED for the OLD model:

### ✅ VERIFIED: Memory Safety
- `msgord_memory_safety_complete` - memory allocation safe
- `msgord_deadlock_prevention_complete` - lock hierarchy prevents deadlock
- `msgord_resource_bounds_complete` - resource usage bounded
- `msgord_complexity_bounds_complete` - k-parameter bounds complexity

### ✅ VERIFIED: General Properties  
- `nat_total_order` - natural number ordering
- `resource_usage_bounded` - basic resource bounds
- `lock_hierarchy_prevents_cycles` - no circular lock ordering

### ⚠️ PARTIALLY VERIFIED: Model Properties (from model.v)
- `msgord_anticone` - old definition (counts ALL pending non-parents)
- `antichain_construction` - builds undirected acyclic graph
- `can_deliver` conditions

## What's MISSING for 2024 Model

The following properties of the 2024 model are NOT proven in existing files:

### 1. Antecone with Explicit Conflict (NEW SEMANTICS)
**File**: `model.v`
**Issue**: `anticone` function still uses old definition (pending non-parents)
**Update Needed**: Replace with counting only CONFLICTING pending non-parents

```coq
(* OLD: Count all pending non-parents *)
Fixpoint anticone (dag : MsgOrd) (msg : GhostMsg) : Z := ...

(* NEW: Count only conflicting pending non-parents *)
Fixpoint anticone (dag : MsgOrd) (msg : GhostMsg) : Z := ...
```

### 2. Resource Key Conflict Detection  
**File**: `model.v`
**Issue**: No definition of `msgord_messages_conflict`
**New Definition Needed**:
```coq
Definition msgord_messages_conflict (a b : GhostMsg) : Prop :=
  (a.gm_resource_count = 0)  (* global barrier *)
  ∨ (b.gm_resource_count = 0)  (* global barrier *)
  ∨ (∃ i j, a.gm_resource_keys[i] = b.gm_resource_keys[j]).
```

### 3. Frontier-Based Parent Selection  
**File**: NONE
**Issue**: No theorem proving frontier parent selection sound
**Needed**: Parent selection never creates cycles

### 4. Anticone Properties Under New Model  
**File**: `msgord_verified_simple.v`
**Issue**: `large_anticone_is_red` uses old anticone
**Needed**: Prove that large anticone (under new definition) leads to RED
**Note**: Actually this theorem is EXPENSIVE to prove with new definition since it involves resource key matching

### 5. MsgOrdSpec Properties  
**File**: NONE
**Issue**: No theorems about caller-provided specs
**Needed**: 
- Spec construction (`msgord_spec_add`) preserves invariants
- Spec determines correct conflict relationships
- Empty spec correctly indicates global barrier

### 6. Concurrent Non-Conflicting Operations  
**File**: NONE
**Issue**: No theory of concurrency for non-conflicting operations
**Needed**: 
- Operations on different resource keys can execute concurrently
- Formalize independence: key1 ≠ key2 → concurrent execution safe

## Recommended Proof File Structure

Create `proofs/msgord/model_2024.v`:
```coq
(* Updated anticone definition for 2024 model *)
Fixpoint anticone (dag : MsgOrd) (msg : GhostMsg) : Z := ...

(* Conflict detection *)
Definition msgord_messages_conflict (a b : GhostMsg) : Prop := ...

(* Proof: 2024 anticone properties *)
Theorem anticone_2024_bounded : forall dag msg, anticone dag msg ≤ k_parameter + MAX_CONCURRENT_CONFLICTS.
Proof. admit.

(* Proof: Frontier parent selection maintains DAG property *)
Theorem frontier_parents_sound : forall dag msg, 
  ∀ parent, parent ∈ msg.gm_parents → 
    exists ancestor in dag, ancestor.gm_id = parent ∧ ancestor.gm_timestamp < msg.gm_timestamp.
Proof. admit.
```

## Migration Strategy

Since `model.v` is already verified, the safest approach is:
1. Create `model_2024.v` with new definitions
2. Create `msgord_2024_properties.v` with new theorems
3. Maintain `model.v` for backward compatibility
4. Gradually migrate theorems (breaking existing ones that depend on old anticone)

## Current Status

- **Old model**: Fully proven (verified on Feb 12, 2026)
- **New 2024 model**: Properties NOT yet formalized
- **Risk**: Changing anticone definition in `model.v` would break existing verified theorems
- **Mitigation**: New file approach preserves existing verification
