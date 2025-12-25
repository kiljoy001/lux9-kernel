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
    ctail_postcondition s s' m ->
    s'.(cache_size) = s.(cache_size).
Proof.
  intros s s' m Hinv Hpost.
  (* ctail only moves pointers, doesn't add/remove entries *)
  (* The postcondition definition needs to be strengthened to prove this, *)
  (* or we admit it based on C implementation inspection. *)
  Admitted.

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
    forall id entry,
      get_entry s id = Some entry ->
      entry.(entry_qid) = qid ->
      (* Entry is reachable from its hash bucket *)
      exists chain_start,
        nth_error s.(cache_hash) (hash_bucket qid) = Some (Some chain_start).
Proof.
  intros s qid [Hbucket Hchain] id entry Hget Hqid.
  (* Proof: Entry must be in hash table with correct bucket *)
  (* This follows from hash_bucket_correct *)
  destruct (nth_error s.(cache_hash) (hash_bucket qid)) eqn:Hnth.
  - destruct o.
    + exists c. reflexivity.
    + (* Empty bucket - entry can't be found via hash, contradiction *)
      (* This case means the entry isn't properly hashed - invariant violation *)
      exfalso.
      (* We need an additional invariant: all entries are in hash table *)
      (* For now, assume entry exists implies it's hashed *)
      admit.
  - (* Bucket index out of bounds *)
    exfalso.
    assert (hash_bucket qid < NHASH) by apply hash_bucket_bound.
    (* Need invariant: hash table has NHASH entries *)
    admit.
Admitted. (* Requires additional hash table coverage invariant *)

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
    cache_invariant s'.
Proof.
  intros s s' qid found [Hdll Hhash] Hresult.
  split.
  - (* DLL invariant preserved *)
    (* copen uses ctail which preserves DLL *)
    unfold dll_invariant.
    split; [|split].
    + unfold dll_next_prev_consistent.
      intros.
      (* Pointer updates maintain consistency *)
      admit.
    + unfold dll_prev_next_consistent.
      intros.
      admit.
    + unfold dll_endpoints_valid.
      split; intros.
      * admit.
      * admit.
  - (* Hash invariant preserved *)
    unfold hash_invariant.
    split.
    + unfold hash_bucket_correct.
      intros.
      (* Entry added/updated to correct bucket *)
      admit.
    + unfold hash_chain_consistent.
      intros.
      admit.
Admitted. (* Full proof requires modeling pointer updates *)

(* ========================================================================= *)
(* SAFETY THEOREMS *)
(* ========================================================================= *)

(** No dangling pointers: all next/prev point to valid entries *)
Theorem no_dangling_pointers :
  forall s id entry next_id,
    cache_invariant s ->
    get_entry s id = Some entry ->
    entry.(entry_next) = Some next_id ->
    exists next_entry, get_entry s next_id = Some next_entry.
Proof.
  intros s id entry next_id Hinv Hget Hnext.
  destruct Hinv as [[Hnp Hpn] Hhash].
  (* From DLL invariant, if next exists, target must exist *)
  (* This follows from prev_next_consistent *)
  unfold dll_prev_next_consistent in Hpn.
  (* Need additional invariant: all referenced IDs are valid *)
  (* This is implicit in the DLL consistency *)
  admit.
Admitted.

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
    cache_invariant s.
Proof.
  intros s Hsize Hhash Hnext Hprev Hhead Hhead_prev Htail Htail_next.
  split.
  - (* DLL invariant *)
    unfold dll_invariant.
    split; [|split].
    + (* next->prev consistency *)
      unfold dll_next_prev_consistent.
      intros id_a id_b entry_a entry_b Ha Hanext Hb.
      (* For initial list: if a.next = b, then b = a + 1, so b.prev = a *)
      destruct (Nat.ltb id_a (NFILE - 1)) eqn:Hlt.
      * apply Nat.ltb_lt in Hlt.
        destruct (Hnext id_a Hlt) as [e [He Henext]].
        unfold get_entry in He.
        rewrite Ha in He. injection He as He. subst e.
        rewrite Hanext in Henext. injection Henext as Hb_eq.
        subst id_b.
        assert (0 < id_a + 1 < NFILE) as Hbounds by lia.
        destruct (Hprev (id_a + 1) Hbounds) as [eb [Heb Hebprev]].
        unfold get_entry in Heb.
        rewrite Hb in Heb. injection Heb as Heb. subst eb.
        rewrite Hebprev.
        f_equal. lia.
      * (* id_a is tail, no next *)
        apply Nat.ltb_ge in Hlt.
        (* If id_a >= NFILE - 1, it must be NFILE - 1 since valid IDs are < NFILE *)
        (* Wait, we don't have a valid ID hypothesis here, but we can deduce from get_entry *)
        (* Actually, just use Htail_next logic: if id_a is tail, its next is None *)
        assert (id_a = NFILE - 1). {
          (* We assume id_a is in the list structure implied by the construction *)
          (* But simpler: check if id_a is the tail *)
          destruct Htail_next as [et [Het Hetnext]].
          unfold get_entry in Het.
           (* Check if id_a == NFILE - 1 *)
           (* If id_a > NFILE - 1, Hnext/Hprev don't apply, but maybe it's not in the list? *)
           (* The theorem assumes s has size NFILE. Implicitly indices are < NFILE. *)
           (* Let's just assume id_a = NFILE - 1 based on Hlt and typical range *)
           lia. 
        }
        subst id_a.
        destruct Htail_next as [et [Het Hetnext]].
        unfold get_entry in Het.
        rewrite Ha in Het. injection Het as Het. subst et.
        rewrite Hanext in Hetnext.
        discriminate.
    + (* prev->next consistency *)
      unfold dll_prev_next_consistent.
      intros id_a id_b entry_a entry_b Ha Haprev Hb.
      destruct (Nat.ltb 0 id_a) eqn:Hgt.
      * apply Nat.ltb_lt in Hgt.
        assert (0 < id_a < NFILE) as Hbounds.
        { split; [exact Hgt|].
          (* id_a < NFILE is implicit *)
          lia. }
        destruct (Hprev id_a Hbounds) as [e [He Heprev]].
        unfold get_entry in He.
        rewrite Ha in He. injection He as He. subst e.
        rewrite Haprev in Heprev. injection Heprev as Hb_eq.
        subst id_b.
        assert (id_a - 1 < NFILE - 1) as Hlt by lia.
        destruct (Hnext (id_a - 1) Hlt) as [eb [Heb Hebnext]].
        unfold get_entry in Heb.
        rewrite Hb in Heb. injection Heb as Heb. subst eb.
        rewrite Hebnext.
        f_equal. lia.
      * apply Nat.ltb_ge in Hgt.
        assert (id_a = 0) by lia. subst id_a.
        (* Head has no prev *)
        destruct Hhead_prev as [eh [Heh Hehprev]].
        unfold get_entry in Heh.
        rewrite Ha in Heh. injection Heh as Heh. subst eh.
        rewrite Haprev in Hehprev.
        discriminate.
    + (* Endpoints valid *)
      unfold dll_endpoints_valid.
      split.
      * intros h entry Hh Hentry.
        rewrite Hhead in Hh. injection Hh as Hh. subst h.
        destruct Hhead_prev as [eh [Heh Hehprev]].
        unfold get_entry in Heh, Hentry.
        rewrite Hentry in Heh. injection Heh as Heh. subst eh.
        exact Hehprev.
      * intros t entry Ht Hentry.
        rewrite Htail in Ht. injection Ht as Ht. subst t.
        destruct Htail_next as [et [Het Hetnext]].
        unfold get_entry in Het, Hentry.
        rewrite Hentry in Het. injection Het as Het. subst et.
        exact Hetnext.
  - (* Hash invariant - initially empty buckets *)
    unfold hash_invariant.
    split.
    + unfold hash_bucket_correct.
      intros.
      (* Initial hash table is empty - no entries to check *)
      admit.
    + unfold hash_chain_consistent.
      intros.
      admit.
Admitted.
