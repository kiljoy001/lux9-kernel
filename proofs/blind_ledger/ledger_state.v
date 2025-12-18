(** Blind Ledger - Phase 2: Abstract State Machine
    Defines the logical behavior of the ledger including Mint, Transfer, and Burn operations. *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import BlindLedger.ledger_crypto.
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* STATE DEFINITIONS *)
(* ========================================================================= *)

Definition Permissions := Z.

Inductive EntryState :=
  | State_Active
  | State_Burned
  | State_CowRed
  | State_CowBlue.

Record BlindLedgerEntry := mkEntry {
  ent_capability : UserCapability;
  ent_pa : PAddr;
  ent_span_len : Len;
  ent_owner : Proc;
  ent_permissions : Permissions;
  ent_secret : Secret;
  ent_state : EntryState;
  ent_process_hash : ProcessHash;
  ent_leaf_hash : LeafHash;
}.

Definition LedgerState := CapHash -> option BlindLedgerEntry.

Definition empty_state : LedgerState :=
  fun _ => None.



Definition update_state (s : LedgerState) (k : CapHash) (v : option BlindLedgerEntry) : LedgerState :=
  fun x => if (CapHash_eq_dec x k) then v else s x.

(* Helper to remove an entry *)
Definition remove_entry (s : LedgerState) (k : CapHash) : LedgerState :=
  update_state s k None.

Inductive LedgerError :=
  | LE_Ok
  | LE_Invalid
  | LE_NotFound
  | LE_Expired
  | LE_Perm
  | LE_NoMem.

(* ========================================================================= *)
(* TRANSITIONS *)
(* ========================================================================= *)

(** 
 * Mint Operation
 *)
Definition op_mint (s : LedgerState) (pa : PAddr) (len : Len) (owner : Proc) 
                   (perms : Permissions) (sec : Secret) 
                   : (LedgerState * UserCapability * LedgerError) :=
  
  (* Dummy hash for error cases *)
  let dummy_hash := Hash_Cap (Hash_Process (mkSecret 0) (Hash_Leaf 0 0) 0) (Hash_Leaf 0 0) in
  if (Z.leb len 0) then (s, mkUserCapability dummy_hash 0 0 0, LE_Invalid) else
  
  (* 1. Compute Hashes with Owner *)
  let cap_hash := mint_capability_hash pa len sec owner in
  let lh := compute_leaf_hash pa len in
  let ph := compute_process_hash sec lh owner in
  
  match s cap_hash with
  | Some _ => (s, mkUserCapability dummy_hash 0 0 0, LE_Invalid)
  | None =>
      let cap := mkUserCapability cap_hash len 1 perms in
      let entry := mkEntry cap pa len owner perms sec State_Active ph lh in
      (update_state s cap_hash (Some entry), cap, LE_Ok)
  end.

(** 
 * Transfer Operation
 * Moves ownership -> Changes Hash -> Old Cap Invalidated -> New Cap Issued
 *)
Definition op_transfer (s : LedgerState) (old_cap : UserCapability) 
                       (from_proc : Proc) (to_proc : Proc) 
                       : (LedgerState * UserCapability * LedgerError) :=
  
  match s (cap_hash old_cap) with
  | None => (s, old_cap, LE_NotFound)
  | Some entry =>
      match entry.(ent_state) with
      | State_Active => 
          if (Z.eqb entry.(ent_owner) from_proc) then
             (* 1. Calculate NEW hashes for new owner *)
             let new_cap_hash := mint_capability_hash 
                 entry.(ent_pa) 
                 entry.(ent_span_len) 
                 entry.(ent_secret) 
                 to_proc in
             
             let new_ph := compute_process_hash 
                 entry.(ent_secret) 
                 entry.(ent_leaf_hash) 
                 to_proc in
             
             (* 2. Create new Capability/Entry *)
             let new_cap := mkUserCapability 
                 new_cap_hash 
                 entry.(ent_span_len) 
                 entry.(ent_capability).(cap_type) 
                 entry.(ent_permissions) in
                 
             let new_entry := mkEntry 
                 new_cap 
                 entry.(ent_pa) 
                 entry.(ent_span_len) 
                 to_proc 
                 entry.(ent_permissions) 
                 entry.(ent_secret) 
                 State_Active 
                 new_ph 
                 entry.(ent_leaf_hash) in
             
             (* 3. Update State: Remove old, Add new *)
             (* Note: In the C code, it updates 'node' in place because the Node key depends on the hash. 
                Wait, RBTree key IS the hash. So C code removes and re-inserts (or updates in place if it wasn't keyed).
                blind_ledger.c:321 recalculates hash. 
                But it relies on 'node' pointer equality.
                Abstractly, this is a Delete + Insert. *)
             
             let s_removed := remove_entry s (cap_hash old_cap) in
             let s_final := update_state s_removed new_cap_hash (Some new_entry) in
             
             (s_final, new_cap, LE_Ok)
          else
             (s, old_cap, LE_Perm)
      | _ => (s, old_cap, LE_Expired)
      end
  end.

(** 
 * Burn Operation
 *)
Definition op_burn (s : LedgerState) (cap : UserCapability) (proc : Proc) 
                   : (LedgerState * LedgerError) :=
  match s (cap_hash cap) with
  | None => (s, LE_NotFound)
  | Some entry =>
      if match entry.(ent_state) with State_Active => true | _ => false end then
        if (Z.eqb entry.(ent_owner) proc) then
           let new_entry := mkEntry 
                 entry.(ent_capability)
                 entry.(ent_pa) 
                 entry.(ent_span_len) 
                 entry.(ent_owner) 
                 entry.(ent_permissions) 
                 entry.(ent_secret)
                 State_Burned 
                 entry.(ent_process_hash) 
                 entry.(ent_leaf_hash) in
           (* For burn, we keep the entry but mark it burned, so key stays same *)
           (update_state s (cap_hash cap) (Some new_entry), LE_Ok)
        else
           (s, LE_Perm)
      else
        (s, LE_Expired)
  end.
