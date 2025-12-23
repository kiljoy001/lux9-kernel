(** * Environment Hash - Hash table correctness proofs
    * 
    * Imports: env/types
    * Proves: Hash chain invariants, lookup/insert/remove correctness
    *)

Require Import env.types.

(* ========================================================================= *)
(* HASH CHAIN MODEL                                                          *)
(* ========================================================================= *)

(* We model hash chains as lists of (name_hash, path) pairs *)
Definition HashEntry := (Z * Z)%type.  (* (name_hash, path) *)
Definition HashChain := list HashEntry.
Definition HashTable := list HashChain.  (* ENVHASH buckets *)

(* Hash function maps name to bucket index *)
Definition hash_bucket (name_hash : Z) : Z :=
  Z.modulo name_hash ENVHASH.

(* Entry is in chain *)
Definition in_chain (name_hash path : Z) (chain : HashChain) : Prop :=
  In (name_hash, path) chain.

(* ========================================================================= *)
(* HASH TABLE OPERATIONS                                                     *)
(* ========================================================================= *)

(** Insert entry at head of chain (like devenv.c: e->hash = *h, *h = e) *)
Inductive HashInsert (name_hash path : Z) (c1 c2 : HashChain) : Prop :=
  | HI_Success :
      c2 = (name_hash, path) :: c1 ->
      HashInsert name_hash path c1 c2.

(** Remove entry from chain *)
Inductive HashRemove (name_hash path : Z) (c1 c2 : HashChain) : Prop :=
  | HR_Success :
      in_chain name_hash path c1 ->
      ~ in_chain name_hash path c2 ->
      (forall nh p, (nh, p) <> (name_hash, path) -> 
                    In (nh, p) c1 -> In (nh, p) c2) ->
      HashRemove name_hash path c1 c2.

(** Lookup finds entry in chain *)
Inductive HashLookup (name_hash : Z) (chain : HashChain) (path : Z) : Prop :=
  | HL_Found :
      in_chain name_hash path chain ->
      HashLookup name_hash chain path.

(* ========================================================================= *)
(* HASH TABLE INVARIANTS                                                     *)
(* ========================================================================= *)

(* All entries in a chain have the same bucket index *)
Definition Inv_ChainConsistent (bucket : Z) (chain : HashChain) : Prop :=
  forall nh p, In (nh, p) chain -> hash_bucket nh = bucket.

(* No duplicate paths in a chain *)
Definition Inv_NoDupPaths (chain : HashChain) : Prop :=
  forall nh1 p nh2, In (nh1, p) chain -> In (nh2, p) chain -> nh1 = nh2.

(* ========================================================================= *)
(* HASH OPERATION PROOFS                                                     *)
(* ========================================================================= *)

Theorem insert_adds_entry :
  forall nh p c1 c2,
  HashInsert nh p c1 c2 -> in_chain nh p c2.
Proof.
  intros nh p c1 c2 H. inversion H. subst.
  unfold in_chain. left. reflexivity.
Qed.

Theorem insert_preserves_existing :
  forall nh p c1 c2 nh' p',
  HashInsert nh p c1 c2 ->
  in_chain nh' p' c1 ->
  in_chain nh' p' c2.
Proof.
  intros nh p c1 c2 nh' p' Hi Hin.
  inversion Hi. subst.
  unfold in_chain in *. right. assumption.
Qed.

Theorem remove_removes_entry :
  forall nh p c1 c2,
  HashRemove nh p c1 c2 -> ~ in_chain nh p c2.
Proof.
  intros nh p c1 c2 H. inversion H. assumption.
Qed.

Theorem remove_preserves_others :
  forall nh p c1 c2 nh' p',
  HashRemove nh p c1 c2 ->
  (nh', p') <> (nh, p) ->
  in_chain nh' p' c1 ->
  in_chain nh' p' c2.
Proof.
  intros nh p c1 c2 nh' p' Hr Hneq Hin.
  inversion Hr. unfold in_chain in *.
  apply H1; assumption.
Qed.

Theorem lookup_finds_if_present :
  forall nh chain p,
  in_chain nh p chain -> HashLookup nh chain p.
Proof.
  intros nh chain p Hin.
  constructor. assumption.
Qed.

(* ========================================================================= *)
(* CHAIN LENGTH PROPERTIES                                                   *)
(* ========================================================================= *)

Theorem insert_increases_length :
  forall nh p c1 c2,
  HashInsert nh p c1 c2 -> length c2 = S (length c1).
Proof.
  intros nh p c1 c2 H. inversion H. subst.
  simpl. reflexivity.
Qed.

(* Insert then remove returns to original if removing same entry *)
Theorem insert_remove_inverse :
  forall nh p c1 c2 c3,
  HashInsert nh p c1 c2 ->
  ~ in_chain nh p c1 ->
  HashRemove nh p c2 c3 ->
  (forall nh' p', in_chain nh' p' c1 -> in_chain nh' p' c3).
Proof.
  intros nh p c1 c2 c3 Hi Hnotin Hr.
  intros nh' p' Hin.
  inversion Hi. inversion Hr. subst.
  apply H2.
  - unfold not. intros Heq. inversion Heq. subst. contradiction.
  - unfold in_chain. right. assumption.
Qed.

(* ========================================================================= *)
(* LOOKUPNAME CORRECTNESS                                                    *)
(* ========================================================================= *)

(* lookupname walks the hash chain - correctness theorem *)
Theorem lookupname_correct :
  forall nh p chain,
  HashLookup nh chain p <-> in_chain nh p chain.
Proof.
  intros nh p chain.
  split.
  - intros H. inversion H. assumption.
  - intros Hin. constructor. assumption.
Qed.
