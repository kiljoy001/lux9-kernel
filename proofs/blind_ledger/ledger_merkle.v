(** Blind Ledger - Phase 3: Merkle Tree Integrity (Symbolic)
    Defines Merkle Tree construction using inductive terms (Term Algebra) 
    to guarantee collision resistance by construction. *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Lia.
Require Import Arith.
Require Import PeanoNat.
Require Import BlindLedger.ledger_crypto.
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* SYMBOLIC MERKLE DEFINITIONS *)
(* ========================================================================= *)

(**
 * In the symbolic model, the "Hash" of a tree node is simply the term 
 * constructed from its children. This models perfect collision resistance.
 *)
Inductive MerkleHash :=
  | MHash_Cap (c : CapHash)
  | MHash_Node (l : MerkleHash) (r : MerkleHash).
  
(**
 * Helper to get the top hash of a symbolic tree
 * In this model, the tree structure IS the hash structure.
 *)
Definition get_merkle_root (t : MerkleHash) : MerkleHash := t.

(**
 * dummy hash for padding/empty cases 
 *)
Definition dummy_cap_hash := Hash_Cap (Hash_Process (mkSecret 0) (Hash_Leaf 0 0) 0) (Hash_Leaf 0 0).
Definition dummy_node := MHash_Cap dummy_cap_hash.

(**
 * Build a tree from a list of Capability Hashes.
 *)
Fixpoint build_level (hashes : list MerkleHash) : list MerkleHash :=
  match hashes with
  | [] => []
  | [t] => [t] (* Odd number, promotes freely *)
  | t1 :: t2 :: rest =>
      let parent := MHash_Node t1 t2 in
      parent :: build_level rest
  end.

Fixpoint build_tree_recursive (leaves : list MerkleHash) (gas : nat) : MerkleHash :=
  match gas with
  | O => match leaves with 
         | [] => dummy_node
         | h :: _ => h 
         end
  | S n =>
      match leaves with
      | [] => dummy_node
      | [root] => root
      | _ => build_tree_recursive (build_level leaves) n
      end
  end.

Definition build_merkle_tree (caps : list CapHash) : MerkleHash :=
  let leaves := map MHash_Cap caps in
  build_tree_recursive leaves (length caps).

(* ========================================================================= *)
(* INTEGRITY THEOREMS (No Axioms) *)
(* ========================================================================= *)



(**
 * Lemma: Deterministic length of build_level
 *)
Lemma length_build_level_aux : forall (n : nat) (l : list MerkleHash), (length l <= n)%nat -> length (build_level l) = Nat.div2 (S (length l)).
Proof.
  induction n as [|n IH].
  - intros l H; destruct l; auto. simpl in H. lia.
  - intros l H. destruct l as [|x xs].
    + simpl. reflexivity.
    + destruct xs as [|y ys].
      * simpl. reflexivity.
      * simpl. f_equal. apply IH. simpl in H. lia.
Qed.

Lemma build_level_length_eq : forall l1 l2,
  length l1 = length l2 ->
  length (build_level l1) = length (build_level l2).
Proof.
  intros l1 l2 H.
  rewrite (length_build_level_aux (length l1) l1); [|auto].
  rewrite (length_build_level_aux (length l2) l2); [|auto].
  rewrite H. reflexivity.
Qed.

(**
 * Theorem: Injectivity of build_level
 *)


Lemma MHash_Node_injective : forall a b c d, MHash_Node a b = MHash_Node c d -> a = c /\ b = d.
Proof. inversion 1; auto. Qed.

Lemma build_level_injective_aux : forall n l1 l2, 
  (length l1 <= n)%nat ->
  length l1 = length l2 ->
  build_level l1 = build_level l2 ->
  l1 = l2.
Proof.
  induction n as [|n IH].
  - intros l1 l2 Hle Hlen Hbeq. 
    destruct l1.
    + destruct l2; [reflexivity | discriminate Hlen].
    + simpl in Hle. lia. (* S n <= 0 False *)
  - intros l1 l2 Hle Hlen Hbeq.
    destruct l1 as [|x1 xs1].
    + destruct l2; [reflexivity | discriminate Hlen].
    + destruct l2 as [|x2 xs2]; [discriminate Hlen | ].
      (* Step *)
      destruct xs1 as [|y1 ys1].
      * destruct xs2 as [|y2 ys2].
        ** simpl in Hbeq. injection Hbeq as Hx. subst. reflexivity.
        ** simpl in Hlen. discriminate Hlen.
      * destruct xs2 as [|y2 ys2].
        ** simpl in Hlen. discriminate Hlen.
        ** (* Recursive case *)
           simpl in Hbeq.
           (* Inversion decomposes Node x1 y1 = Node x2 y2 AND tail equality *)
           inversion Hbeq. subst.
           
           f_equal. f_equal.
           apply IH.
           *** simpl in Hle. simpl in Hle. lia.
           *** simpl in Hlen. injection Hlen. auto.
           *** assumption.
Qed.

Lemma build_level_injective : forall l1 l2,
  length l1 = length l2 ->
  build_level l1 = build_level l2 ->
  l1 = l2.
Proof.
  intros l1 l2 Hlen Hbeq.
  apply build_level_injective_aux with (n:=length l1); auto.
Qed.




Lemma ind_div2 : forall P : nat -> Prop,
  P 0%nat ->
  P 1%nat ->
  (forall n, P n -> P (S (S n))) ->
  forall n, P n.
Proof.
  intros P H0 H1 Hstep n.
  cut (P n /\ P (S n)).
  - intuition.
  - induction n.
    + auto.
    + destruct IHn. split.
      * assumption.
      * apply Hstep. assumption.
Qed.





(* Use stdlib's Nat.lt_div2 : 0 < n -> div2 n < n *)
Lemma local_div2_lt : forall n, (n <> 0 -> Nat.div2 n < n)%nat.
Proof.
  intros n Hn.
  apply Nat.lt_div2.
  lia.
Qed.

Lemma build_level_size_bound : forall l n,
  (2 <= length l)%nat ->
  (length l <= S n)%nat ->
  (length (build_level l) <= n)%nat.
Proof.
  intros l n Hge Hle.
  rewrite length_build_level_aux with (n:=length l); [|auto].
  destruct l as [|x xs].
  - simpl in Hge. lia.
  - destruct xs as [|y ys].
    + simpl in Hge. lia.
    + clear Hge. simpl. simpl.
      (* Need: S (div2 (S (length ys))) <= n *)
      (* Have: S (S (length ys)) <= S n *)
      simpl in Hle.
      (* Hle: S (S (length ys)) <= S n *)
      (* Use Nat.le_div2: div2 (S m) <= m *)
      pose proof (Nat.le_div2 (length ys)) as Hdiv.
      (* Hdiv: div2 (S (length ys)) <= length ys *)
      (* From Hle: S (length ys) <= n, so length ys < n *)
      (* We need: S (div2 (S (length ys))) <= n *)
      (* From Hdiv: div2 (S (length ys)) <= length ys *)
      (* So S (div2 (S (length ys))) <= S (length ys) <= n by Hle *)
      apply Nat.le_trans with (m := S (length ys)).
      * apply le_n_S. exact Hdiv.
      * apply Nat.succ_le_mono in Hle. exact Hle.
Qed.

Lemma build_tree_injective : forall n l1 l2,
  length l1 = length l2 ->
  build_tree_recursive l1 n = build_tree_recursive l2 n ->
  (n >= length l1)%nat -> (* Sufficient gas *)
  l1 = l2.
Proof.
  induction n; intros l1 l2 Hlen Hres Hgas.
  - simpl in Hgas. 
    destruct l1.
    + destruct l2; [reflexivity | discriminate Hlen].
    + simpl in Hgas. lia.
  - simpl in Hres.
    destruct l1 as [|x1 xs1]; destruct l2 as [|x2 xs2].
    + reflexivity. 
    + discriminate Hlen.
    + discriminate Hlen.
    + destruct xs1 as [|y1 ys1].
      * destruct xs2. 
        ** rewrite Hres. reflexivity.
        ** discriminate Hlen.
      * destruct xs2 as [|y2 ys2].
        ** discriminate Hlen.
        ** (* Both have >= 2 elements *)
           apply build_level_injective with (l2:=x2::y2::ys2).
           *** exact Hlen.
           *** apply IHn.
               **** apply build_level_length_eq. exact Hlen.
               **** exact Hres.
               **** apply build_level_size_bound.
                    ***** simpl. lia. (* 2 <= 2+length ys1 *)
                    ***** simpl in Hgas. exact Hgas.
Qed.

(* Helper: MHash_Cap is injective *)
Lemma MHash_Cap_injective : forall c1 c2, MHash_Cap c1 = MHash_Cap c2 -> c1 = c2.
Proof.
  intros c1 c2 H.
  injection H as H'. exact H'.
Qed.

(* Helper: map is injective if f is injective *)
Lemma map_injective_aux : forall (A B : Type) (f : A -> B) (l1 l2 : list A),
  (forall x y, f x = f y -> x = y) ->
  map f l1 = map f l2 ->
  l1 = l2.
Proof.
  intros A B f l1.
  induction l1 as [|h1 t1 IH].
  - intros l2 Hf Hmap.
    destruct l2; [reflexivity | discriminate Hmap].
  - intros l2 Hf Hmap.
    destruct l2 as [|h2 t2]; [discriminate Hmap|].
    simpl in Hmap. injection Hmap as Hhead Htail.
    f_equal.
    + apply Hf. exact Hhead.
    + apply IH; assumption.
Qed.

Theorem build_merkle_tree_injective : forall c1 c2,
  length c1 = length c2 ->
  build_merkle_tree c1 = build_merkle_tree c2 ->
  c1 = c2.
Proof.
  intros c1 c2 Hlen Htree.
  unfold build_merkle_tree in Htree.
  (* First show that map MHash_Cap c1 = map MHash_Cap c2 uses build_tree_injective *)
  assert (Hmaplen: length (map MHash_Cap c1) = length (map MHash_Cap c2)).
  { do 2 rewrite length_map. exact Hlen. }
  (* Rewrite to make types match *)
  rewrite Hlen in Htree.
  assert (Hmapeq: map MHash_Cap c1 = map MHash_Cap c2).
  { apply build_tree_injective with (n := length c2).
    - exact Hmaplen.
    - exact Htree.
    - rewrite length_map. lia. }
  (* Now derive c1 = c2 from map MHash_Cap c1 = map MHash_Cap c2 *)
  apply map_injective_aux with (f := MHash_Cap); [exact MHash_Cap_injective | exact Hmapeq].
Qed.

(**
 * Main Integrity Property (Contrapositive of Injectivity)
 *)
Theorem root_sensitivity : forall caps1 caps2,
  caps1 <> caps2 ->
  length caps1 = length caps2 ->
  build_merkle_tree caps1 <> build_merkle_tree caps2.
Proof.
  intros c1 c2 Hneq Hlen Hcontra.
  apply Hneq.
  apply build_merkle_tree_injective; assumption.
Qed.
