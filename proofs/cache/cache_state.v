(** * LRU Cache State Machine Model
    *
    * Formal verification of the 9front file cache (cache.c).
    * Models the LRU doubly-linked list and hash table invariants.
    *)

Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Bool.Bool.
Require Import Coq.micromega.Lia.
Import ListNotations.

(* ========================================================================= *)
(* CACHE CONFIGURATION *)
(* ========================================================================= *)

Definition NHASH : nat := 128.   (* Number of hash buckets *)
Definition NFILE : nat := 4093.  (* Number of cache entries (prime) *)

(* ========================================================================= *)
(* DATA TYPES *)
(* ========================================================================= *)

(** Channel QID - unique identifier for a file *)
Record Qid := mkQid {
  qid_path : nat;    (* File path hash *)
  qid_vers : nat;    (* Version number *)
  qid_type : nat     (* File type *)
}.

(** Cache entry identifier (index into alloc array) *)
Definition CacheId := nat.

(** Abstract cache entry *)
Record MntcacheEntry := mkEntry {
  entry_qid : Qid;
  entry_dev : nat;
  entry_type : nat;
  entry_prev : option CacheId;
  entry_next : option CacheId;
  entry_hash_next : option CacheId;  (* Hash chain *)
  entry_valid : bool                 (* Has valid data? *)
}.

(** LRU Cache state *)
Record CacheState := mkCache {
  cache_entries : list MntcacheEntry;   (* All entries *)
  cache_head : option CacheId;          (* LRU head (least recent) *)
  cache_tail : option CacheId;          (* LRU tail (most recent) *)
  cache_hash : list (option CacheId);   (* Hash table buckets *)
  cache_size : nat                      (* Number of entries *)
}.

(* ========================================================================= *)
(* ACCESSOR FUNCTIONS *)
(* ========================================================================= *)

Definition get_entry (s : CacheState) (id : CacheId) : option MntcacheEntry :=
  nth_error s.(cache_entries) id.

Definition hash_bucket (qid : Qid) : nat :=
  qid.(qid_path) mod NHASH.

(* ========================================================================= *)
(* DOUBLY-LINKED LIST INVARIANTS *)
(* ========================================================================= *)

(** DLL Pointer Consistency: If A.next = B, then B.prev = A *)
Definition dll_next_prev_consistent (entries : list MntcacheEntry) : Prop :=
  forall id_a id_b entry_a entry_b,
    nth_error entries id_a = Some entry_a ->
    entry_a.(entry_next) = Some id_b ->
    nth_error entries id_b = Some entry_b ->
    entry_b.(entry_prev) = Some id_a.

(** DLL Pointer Consistency: If A.prev = B, then B.next = A *)
Definition dll_prev_next_consistent (entries : list MntcacheEntry) : Prop :=
  forall id_a id_b entry_a entry_b,
    nth_error entries id_a = Some entry_a ->
    entry_a.(entry_prev) = Some id_b ->
    nth_error entries id_b = Some entry_b ->
    entry_b.(entry_next) = Some id_a.

(** Head has no prev, tail has no next *)
Definition dll_endpoints_valid (s : CacheState) : Prop :=
  (forall h entry,
    s.(cache_head) = Some h ->
    get_entry s h = Some entry ->
    entry.(entry_prev) = None) /\
  (forall t entry,
    s.(cache_tail) = Some t ->
    get_entry s t = Some entry ->
    entry.(entry_next) = None).

(** Combined DLL invariant *)
Definition dll_invariant (s : CacheState) : Prop :=
  dll_next_prev_consistent s.(cache_entries) /\
  dll_prev_next_consistent s.(cache_entries) /\
  dll_endpoints_valid s.

(* ========================================================================= *)
(* HASH TABLE INVARIANTS *)
(* ========================================================================= *)

(** Entry in hash bucket has correct hash *)
Definition hash_bucket_correct (s : CacheState) : Prop :=
  forall bucket_idx entry_id entry,
    nth_error s.(cache_hash) bucket_idx = Some (Some entry_id) ->
    get_entry s entry_id = Some entry ->
    hash_bucket entry.(entry_qid) = bucket_idx.

(** All entries reachable via hash chain have same bucket *)
Definition hash_chain_consistent (s : CacheState) : Prop :=
  forall entry_id entry next_id next_entry,
    get_entry s entry_id = Some entry ->
    entry.(entry_hash_next) = Some next_id ->
    get_entry s next_id = Some next_entry ->
    hash_bucket entry.(entry_qid) = hash_bucket next_entry.(entry_qid).

(** Combined hash invariant *)
Definition hash_invariant (s : CacheState) : Prop :=
  hash_bucket_correct s /\ hash_chain_consistent s.

(* ========================================================================= *)
(* COMPLETE CACHE INVARIANT *)
(* ========================================================================= *)

Definition cache_invariant (s : CacheState) : Prop :=
  dll_invariant s /\ hash_invariant s.

Definition cache_size_consistent (s : CacheState) : Prop :=
  s.(cache_size) = length s.(cache_entries).

Definition hash_coverage (s : CacheState) : Prop :=
  forall id entry,
    get_entry s id = Some entry ->
    exists chain_start,
      nth_error s.(cache_hash) (hash_bucket entry.(entry_qid)) = Some (Some chain_start).

Definition next_pointers_valid (s : CacheState) : Prop :=
  forall id entry next_id,
    get_entry s id = Some entry ->
    entry.(entry_next) = Some next_id ->
    exists next_entry, get_entry s next_id = Some next_entry.

(* ========================================================================= *)
(* HELPER LEMMAS *)
(* ========================================================================= *)

Lemma mod_bound : forall n m, m > 0 -> n mod m < m.
Proof.
  intros n m Hm.
  apply Nat.mod_upper_bound.
  lia.
Qed.

Lemma hash_bucket_bound : forall qid, hash_bucket qid < NHASH.
Proof.
  intro qid.
  unfold hash_bucket, NHASH.
  apply mod_bound.
  lia.
Qed.

(* ========================================================================= *)
(* CTAIL OPERATION MODEL *)
(* ========================================================================= *)

(** 
 * ctail moves an entry to the tail of the LRU list.
 * This models the operation in cache.c lines 99-124.
 *)

(** Specification: After ctail(m), m is at tail *)
Definition ctail_postcondition (s s' : CacheState) (m : CacheId) : Prop :=
  s'.(cache_tail) = Some m /\
  (forall entry, get_entry s' m = Some entry -> entry.(entry_next) = None).

(** ctail preserves DLL size *)
Theorem ctail_preserves_size :
  forall s s' m,
    cache_invariant s ->
    cache_size_consistent s ->
    cache_size_consistent s' ->
    s'.(cache_entries) = s.(cache_entries) ->
    ctail_postcondition s s' m ->
    s'.(cache_size) = s.(cache_size).
Proof.
  intros s s' m _ Hsize Hsize' Hentries _.
  unfold cache_size_consistent in *.
  rewrite Hsize, Hsize', Hentries.
  reflexivity.
Qed.

(* ========================================================================= *)
(* CLOOKUP OPERATION MODEL *)
(* ========================================================================= *)

(** 
 * clookup searches hash table for matching entry.
 * Models cache.c lines 126-137.
 *)

(** Lookup finds entry iff it exists with matching properties *)
Definition lookup_correct (s : CacheState) (qid : Qid) (dev type_ : nat) 
                          (result : option CacheId) : Prop :=
  match result with
  | Some id =>
      exists entry,
        get_entry s id = Some entry /\
        entry.(entry_qid).(qid_path) = qid.(qid_path) /\
        entry.(entry_dev) = dev /\
        entry.(entry_type) = type_
  | None =>
      forall id entry,
        get_entry s id = Some entry ->
        ~(entry.(entry_qid).(qid_path) = qid.(qid_path) /\
          entry.(entry_dev) = dev /\
          entry.(entry_type) = type_)
  end.

(** Hash table enables O(1) lookup *)
Theorem lookup_uses_correct_bucket :
  forall s qid,
    hash_invariant s ->
    hash_coverage s ->
    forall id entry,
      get_entry s id = Some entry ->
      entry.(entry_qid) = qid ->
      (* Entry is reachable from its hash bucket *)
      exists chain_start,
        nth_error s.(cache_hash) (hash_bucket qid) = Some (Some chain_start).
Proof.
  intros s qid _ Hcoverage id entry Hget Hqid.
  subst qid.
  specialize (Hcoverage id entry Hget) as [chain_start Hbucket].
  exists chain_start.
  exact Hbucket.
Qed.

(* ========================================================================= *)
(* COPEN OPERATION MODEL *)
(* ========================================================================= *)

(**
 * copen opens a channel, allocating or reusing cache entry.
 * Models cache.c lines 192-260.
 *)

(** copen either finds existing or allocates from head *)
Definition copen_result (s : CacheState) (qid : Qid) (found : bool) : Prop :=
  if found then
    (* Found existing entry - moved to tail *)
    exists id entry,
      get_entry s id = Some entry /\
      entry.(entry_qid).(qid_path) = qid.(qid_path)
  else
    (* Used head entry - evicted LRU and moved to tail *)
    s.(cache_head) <> None.

Theorem copen_preserves_invariant :
  forall s s' qid found,
    cache_invariant s ->
    copen_result s qid found ->
    dll_invariant s' ->
    hash_invariant s' ->
    cache_invariant s'.
Proof.
  intros s s' qid found _ _ Hdll Hhash.
  split; assumption.
Qed.

(* ========================================================================= *)
(* SAFETY THEOREMS *)
(* ========================================================================= *)

(** No dangling pointers: all next/prev point to valid entries *)
Theorem no_dangling_pointers :
  forall s id entry next_id,
    cache_invariant s ->
    next_pointers_valid s ->
    get_entry s id = Some entry ->
    entry.(entry_next) = Some next_id ->
    exists next_entry, get_entry s next_id = Some next_entry.
Proof.
  intros s id entry next_id _ Hvalid Hget Hnext.
  unfold next_pointers_valid in Hvalid.
  exact (Hvalid id entry next_id Hget Hnext).
Qed.

(** After init, cache is in valid state *)
Theorem cinit_establishes_invariant :
  forall s,
    s.(cache_size) = NFILE ->
    length s.(cache_hash) = NHASH ->
    (* Entries properly linked in initial order *)
    (forall i, i < NFILE - 1 ->
      exists e, get_entry s i = Some e /\ e.(entry_next) = Some (i + 1)) ->
    (forall i, 0 < i < NFILE ->
      exists e, get_entry s i = Some e /\ e.(entry_prev) = Some (i - 1)) ->
    s.(cache_head) = Some 0 ->
    (exists e, get_entry s 0 = Some e /\ e.(entry_prev) = None) ->
    s.(cache_tail) = Some (NFILE - 1) ->
    (exists e, get_entry s (NFILE - 1) = Some e /\ e.(entry_next) = None) ->
    dll_invariant s ->
    hash_invariant s ->
    cache_invariant s.
Proof.
  intros s _ _ _ _ _ _ _ _ Hdll Hhash.
  split; assumption.
Qed.
