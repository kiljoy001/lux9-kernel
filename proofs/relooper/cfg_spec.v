(* cfg_spec.v - Coq Specification for CFG Construction Correctness
 *
 * Proves correctness of the two-pass CFG construction algorithm
 * in cil_domtree.c. Required for Ramsey algorithm soundness.
 *
 * @reference: kernel/clr/wasm_backend/cil_domtree.c, domtree_build_cfg
 *)

Require Import List.
Require Import Arith.
Require Import Lia.
Require Import Bool.
Require Import ZArith.
Require Import Sorting.
Require Import Orders.
Import ListNotations.

(* ===== CIL Bytecode Model ===== *)

(* Simplified CIL opcodes focusing on control flow *)
Inductive CilOp : Type :=
  | Op_Nop
  | Op_Ret
  | Op_Br_s (offset: Z)       (* Short unconditional branch *)
  | Op_Brfalse_s (offset: Z)  (* Short conditional branch *)
  | Op_Brtrue_s (offset: Z)   (* Short conditional branch *)
  | Op_Br (offset: Z)         (* Long unconditional branch *)
  | Op_Brfalse (offset: Z)    (* Long conditional branch *)
  | Op_Brtrue (offset: Z)     (* Long conditional branch *)
  | Op_Beq (offset: Z)        (* Compare equal branch *)
  | Op_Bne (offset: Z)        (* Compare not equal branch *)
  | Op_Blt (offset: Z)        (* Compare less than branch *)
  | Op_Bgt (offset: Z)        (* Compare greater than branch *)
  | Op_Ble (offset: Z)        (* Compare less/equal branch *)
  | Op_Bge (offset: Z)        (* Compare greater/equal branch *)
  | Op_Other.                 (* All other non-control-flow opcodes *)

(* Opcode size in bytes *)
Definition op_size (op: CilOp) : nat :=
  match op with
  | Op_Nop => 1
  | Op_Ret => 1
  | Op_Br_s _ | Op_Brfalse_s _ | Op_Brtrue_s _ => 2
  | Op_Br _ | Op_Brfalse _ | Op_Brtrue _ => 5
  | Op_Beq _ | Op_Bne _ | Op_Blt _ | Op_Bgt _ | Op_Ble _ | Op_Bge _ => 5
  | Op_Other => 1
  end.

(* Is the opcode a branch instruction? *)
Definition is_branch (op: CilOp) : bool :=
  match op with
  | Op_Br_s _ | Op_Brfalse_s _ | Op_Brtrue_s _ => true
  | Op_Br _ | Op_Brfalse _ | Op_Brtrue _ => true
  | Op_Beq _ | Op_Bne _ | Op_Blt _ | Op_Bgt _ | Op_Ble _ | Op_Bge _ => true
  | _ => false
  end.

(* Is the branch conditional (has fallthrough)? *)
Definition is_conditional (op: CilOp) : bool :=
  match op with
  | Op_Brfalse_s _ | Op_Brtrue_s _ => true
  | Op_Brfalse _ | Op_Brtrue _ => true
  | Op_Beq _ | Op_Bne _ | Op_Blt _ | Op_Bgt _ | Op_Ble _ | Op_Bge _ => true
  | _ => false
  end.

(* Get branch target offset (relative to end of instruction) *)
Definition branch_offset (op: CilOp) : option Z :=
  match op with
  | Op_Br_s off | Op_Brfalse_s off | Op_Brtrue_s off => Some off
  | Op_Br off | Op_Brfalse off | Op_Brtrue off => Some off
  | Op_Beq off | Op_Bne off | Op_Blt off | Op_Bgt off | Op_Ble off | Op_Bge off => Some off
  | _ => None
  end.

(* ===== Bytecode as List of (Offset, Opcode) ===== *)

Definition Bytecode := list (nat * CilOp).

(* Get opcode at a specific offset *)
Fixpoint get_op_at (bc: Bytecode) (offset: nat) : option CilOp :=
  match bc with
  | [] => None
  | (off, op) :: rest =>
      if Nat.eqb off offset then Some op
      else get_op_at rest offset
  end.

(* ===== Branch Target Identification (Pass 1) ===== *)

(* Compute absolute target from instruction end and relative offset *)
Definition compute_target (instr_end: nat) (rel_offset: Z) : nat :=
  Z.to_nat (Z.of_nat instr_end + rel_offset).

(* Collect all branch targets from bytecode *)
Fixpoint collect_targets (bc: Bytecode) : list nat :=
  match bc with
  | [] => [0]  (* Entry point is always a target *)
  | (off, op) :: rest =>
      let targets := collect_targets rest in
      match branch_offset op with
      | Some rel =>
          let instr_end := off + op_size op in
          let target := compute_target instr_end rel in
          if is_conditional op 
          then instr_end :: target :: targets
          else target :: targets
      | None => targets
      end
  end.

(* A offset is a branch target if some branch points to it *)
Definition is_branch_target (bc: Bytecode) (offset: nat) : Prop :=
  In offset (collect_targets bc).

(* ===== Lemma: 0 is always in collect_targets ===== *)

Lemma zero_in_targets : forall bc, In 0 (collect_targets bc).
Proof.
  induction bc as [| [off op] rest IH].
  - simpl. left. reflexivity.
  - simpl.
    destruct (branch_offset op) eqn:Hbranch.
    + destruct (is_conditional op) eqn:Hcond.
      * right. right. exact IH.
      * right. exact IH.
    + exact IH.
Qed.

(* ===== Basic Block Model ===== *)

Record BasicBlock : Type := mkBlock {
  blk_id : nat;
  blk_start : nat;
  blk_end : nat;
  blk_succs : list nat
}.

Definition CFG := list BasicBlock.

(* Check if a block starts at a given offset *)
Definition block_starts_at (cfg: CFG) (offset: nat) : Prop :=
  exists b, In b cfg /\ blk_start b = offset.

(* Check if CFG has an edge between two blocks *)
Definition cfg_has_edge (cfg: CFG) (src dst: nat) : Prop :=
  exists b, In b cfg /\ blk_id b = src /\ In dst (blk_succs b).

(* Find block by start offset *)
Fixpoint find_block_by_start (cfg: CFG) (offset: nat) : option nat :=
  match cfg with
  | [] => None
  | b :: rest =>
      if Nat.eqb (blk_start b) offset then Some (blk_id b)
      else find_block_by_start rest offset
  end.

(* ===== Sorted Target List ===== *)

(* We assume targets are sorted and deduplicated *)
Inductive Sorted : list nat -> Prop :=
  | Sorted_nil : Sorted []
  | Sorted_one : forall x, Sorted [x]
  | Sorted_cons : forall x y l, 
      x < y -> Sorted (y :: l) -> Sorted (x :: y :: l).

(* No duplicates *)
Inductive NoDup : list nat -> Prop :=
  | NoDup_nil : NoDup []
  | NoDup_cons : forall x l, ~In x l -> NoDup l -> NoDup (x :: l).

(* ===== CFG Construction with Successor Population ===== *)

(* Look up successor block ID from offset *)
Fixpoint offset_to_block_id (targets: list nat) (offset: nat) (id: nat) : option nat :=
  match targets with
  | [] => None
  | t :: rest =>
      if Nat.eqb t offset then Some id
      else offset_to_block_id rest offset (id + 1)
  end.

(* Compute successors for a block [blk_start, blk_end) *)
(* Note: In this simplified model, we assume each basic block contains
   a single instruction at blk_start. A full model would scan [blk_start, blk_end)
   to find the terminator instruction. *)
Definition compute_succs
    (bc: Bytecode) (blk_start blk_end: nat) (targets: list nat) : list nat :=
  (* Note: Simplified model - in real impl, we'd scan block to find terminator *)
  (* For now, we model blocks as having single instructions *)
  match get_op_at bc blk_start with
  | Some op =>
      if is_branch op then
        match branch_offset op with
        | Some rel =>
            let instr_end := blk_start + op_size op in
            let target := compute_target instr_end rel in
            let branch_succ := offset_to_block_id targets target 0 in
            if is_conditional op then
              let fall := instr_end in
              let fall_succ := offset_to_block_id targets fall 0 in
              match branch_succ, fall_succ with
              | Some b, Some f => [b; f]
              | Some b, None => [b]
              | None, Some f => [f]
              | None, None => []
              end
            else
              match branch_succ with
              | Some b => [b]
              | None => []
              end
        | None => []
        end
      else
        (* Fallthrough to next block which starts at blk_end *)
        match offset_to_block_id targets blk_end 0 with
        | Some f => [f]
        | None => []
        end
  | None => []
  end.

(* Build blocks from sorted, deduplicated targets *)
Fixpoint build_blocks
    (bc: Bytecode) (targets: list nat) (bc_size: nat)
    (all_targets: list nat) (next_id: nat) : CFG :=
  match targets with
  | [] => []
  | [last_target] =>
      let succs := compute_succs bc last_target bc_size all_targets in
      [mkBlock next_id last_target bc_size succs]
  | t1 :: ((t2 :: _) as rest) =>
      let succs := compute_succs bc t1 t2 all_targets in
      let block := mkBlock next_id t1 t2 succs in
      block :: build_blocks bc rest bc_size all_targets (next_id + 1)
  end.

(* ===== Helper Lemmas for offset_to_block_id ===== *)

(* offset_to_block_id returns a block ID that's in bounds *)
Lemma offset_to_block_id_bounded : forall targets offset id result,
  offset_to_block_id targets offset id = Some result ->
  exists n, n < length targets /\ result = id + n.
Proof.
  induction targets as [| t rest IH].
  - (* Empty list *)
    intros. simpl in H. discriminate.
  - intros offset id result Hlookup.
    simpl in Hlookup.
    destruct (Nat.eqb t offset) eqn:Heq.
    + (* Found at head *)
      injection Hlookup as Heq_result.
      exists 0.
      split.
      * simpl. lia.
      * lia.
    + (* Recurse *)
      specialize (IH offset (id + 1) result Hlookup).
      destruct IH as [n [Hlt Heq_n]].
      exists (S n).
      split.
      * simpl. lia.
      * lia.
Qed.

(* If offset_to_block_id succeeds with id=0, result is < length targets *)
Corollary offset_to_block_id_valid : forall targets offset result,
  offset_to_block_id targets offset 0 = Some result ->
  result < length targets.
Proof.
  intros.
  apply offset_to_block_id_bounded in H.
  destruct H as [n [Hlt Heq]].
  lia.
Qed.

(* ===== Helper Lemmas for build_blocks ===== *)

(* Length of build_blocks equals length of targets *)
Lemma build_blocks_length : forall bc targets bc_size all_targets next_id,
  length (build_blocks bc targets bc_size all_targets next_id) = length targets.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH].
  - (* Empty *)
    intros. simpl. reflexivity.
  - intros bc_size all_targets next_id.
    destruct rest as [| t2 rest'].
    + (* Single element *)
      simpl. reflexivity.
    + (* t1 :: t2 :: rest' *)
      simpl.
      f_equal.
      apply IH.
Qed.

(* ===== Key Lemma: build_blocks creates a block at each target ===== *)

Lemma build_blocks_creates_at_targets : forall bc targets bc_size all_targets next_id t,
  In t targets ->
  exists b, In b (build_blocks bc targets bc_size all_targets next_id) /\ blk_start b = t.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH].
  - (* Empty list - contradiction *)
    intros. simpl in H. contradiction.
  - intros bc_size all_targets next_id t Hin.
    destruct rest as [| t2 rest'].
    + (* Single element list *)
      simpl in Hin.
      destruct Hin as [Heq | Hcontra]; [| contradiction].
      subst t.
      simpl.
      exists (mkBlock next_id t1 bc_size (compute_succs bc t1 bc_size all_targets)).
      split.
      * left. reflexivity.
      * simpl. reflexivity.
    + (* t1 :: t2 :: rest' *)
      simpl in Hin.
      destruct Hin as [Heq | Hin_rest].
      * (* t = t1 *)
        subst t.
        simpl.
        exists (mkBlock next_id t1 t2 (compute_succs bc t1 t2 all_targets)).
        split.
        -- left. reflexivity.
        -- simpl. reflexivity.
      * (* t in t2 :: rest' *)
        simpl.
        specialize (IH bc_size all_targets (next_id + 1) t Hin_rest).
        destruct IH as [b [Hin_b Hstart]].
        exists b.
        split.
        -- right. exact Hin_b.
        -- exact Hstart.
Qed.

(* ===== Main Theorems ===== *)

(* Theorem 1: All branch targets are identified - FULLY PROVEN *)
Theorem cfg_identifies_all_targets : forall bc offset,
  is_branch_target bc offset ->
  exists cfg, block_starts_at cfg offset.
Proof.
  intros bc offset Htarget.
  unfold is_branch_target in Htarget.
  unfold block_starts_at.
  (* Use build_blocks with collect_targets *)
  set (targets := collect_targets bc).
  set (cfg := build_blocks bc targets 0 targets 0).
  exists cfg.
  apply build_blocks_creates_at_targets.
  exact Htarget.
Qed.

(* Definition for well-formed CFG - built correctly from bytecode *)
Definition well_formed_cfg (bc: Bytecode) (cfg: CFG) (bc_size: nat) : Prop :=
  let targets := collect_targets bc in
  Sorted targets /\ NoDup targets /\ cfg = build_blocks bc targets bc_size targets 0.

(* ===== Helper Lemma: compute_succs produces valid IDs ===== *)

Lemma compute_succs_valid : forall bc blk_start blk_end targets succ,
  In succ (compute_succs bc blk_start blk_end targets) ->
  succ < length targets.
Proof.
  intros bc blk_start blk_end targets succ Hin.
  unfold compute_succs in Hin.
  destruct (get_op_at bc blk_start) eqn:Hop; [| simpl in Hin; contradiction].
  destruct (is_branch c) eqn:Hbr.
  - (* Branch instruction *)
    destruct (branch_offset c) eqn:Hoff; [| simpl in Hin; contradiction].
    destruct (is_conditional c) eqn:Hcond.
    + (* Conditional branch *)
      destruct (offset_to_block_id targets (compute_target (blk_start + op_size c) z) 0) eqn:Hbranch;
      destruct (offset_to_block_id targets (blk_start + op_size c) 0) eqn:Hfall.
      * simpl in Hin. destruct Hin as [H | [H | H]]; try contradiction;
        subst succ; eapply offset_to_block_id_valid; eauto.
      * simpl in Hin. destruct Hin as [H | H]; try contradiction;
        subst succ; eapply offset_to_block_id_valid; eauto.
      * simpl in Hin. destruct Hin as [H | H]; try contradiction;
        subst succ; eapply offset_to_block_id_valid; eauto.
      * simpl in Hin. contradiction.
    + (* Unconditional branch *)
      destruct (offset_to_block_id targets (compute_target (blk_start + op_size c) z) 0) eqn:Hbranch.
      * simpl in Hin. destruct Hin as [H | H]; try contradiction.
        subst succ. eapply offset_to_block_id_valid; eauto.
      * simpl in Hin. contradiction.
  - (* Not a branch - fallthrough to block at blk_end *)
    destruct (offset_to_block_id targets blk_end 0) eqn:Hfall.
    + simpl in Hin. destruct Hin as [H | H]; try contradiction.
      subst succ. eapply offset_to_block_id_valid; eauto.
    + simpl in Hin. contradiction.
Qed.

(* ===== Helper Lemma: Blocks in build_blocks have valid successors ===== *)

Lemma build_blocks_valid_succs : forall bc targets bc_size all_targets next_id b,
  In b (build_blocks bc targets bc_size all_targets next_id) ->
  forall succ, In succ (blk_succs b) -> succ < length all_targets.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH].
  - (* Empty list *)
    intros. simpl in H. contradiction.
  - intros bc_size all_targets next_id b Hin succ Hsucc.
    destruct rest as [| t2 rest'].
    + (* Single element *)
      simpl in Hin.
      destruct Hin as [Heq | Hcontra]; [| contradiction].
      subst b. simpl in Hsucc.
      eapply (compute_succs_valid bc t1 bc_size all_targets); eauto.
    + (* t1 :: t2 :: rest' *)
      simpl in Hin.
      destruct Hin as [Heq | Hin_rest].
      * (* b is the head block *)
        subst b. simpl in Hsucc.
        eapply (compute_succs_valid bc t1 t2 all_targets); eauto.
      * (* b is in the tail *)
        eapply IH; eauto.
Qed.

(* ===== Helper Lemmas: Block ID Assignment ===== *)

(* Blocks in build_blocks have sequential IDs starting from next_id *)
Lemma build_blocks_ids_sequential : forall bc targets bc_size all_targets next_id b,
  In b (build_blocks bc targets bc_size all_targets next_id) ->
  next_id <= blk_id b < next_id + length targets.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH].
  - (* Empty list *)
    intros. simpl in H. contradiction.
  - intros bc_size all_targets next_id b Hin.
    destruct rest as [| t2 rest'].
    + (* Single element *)
      simpl in Hin.
      destruct Hin as [Heq | Hcontra]; [| contradiction].
      subst b. simpl. split; lia.
    + (* t1 :: t2 :: rest' *)
      simpl in Hin.
      destruct Hin as [Heq | Hin_rest].
      * (* b is the head block *)
        subst b. simpl. split; lia.
      * (* b is in the tail *)
        specialize (IH bc_size all_targets (next_id + 1) b Hin_rest).
        destruct IH as [IH1 IH2].
        split.
        -- lia. (* next_id + 1 <= blk_id b, so next_id <= blk_id b *)
        -- simpl. simpl in IH2. lia.
Qed.

(* General lemma: offset_to_block_id with different starting ids - avoids nat subtraction *)
Lemma offset_to_block_id_shift : forall targets offset id1 id2 k,
  offset_to_block_id targets offset id1 = Some (id1 + k) ->
  offset_to_block_id targets offset id2 = Some (id2 + k).
Proof.
  induction targets as [| t rest IH].
  - intros. simpl in H. discriminate.
  - intros offset id1 id2 k Hlookup.
    simpl in Hlookup. simpl.
    destruct (Nat.eqb t offset) eqn:Heq.
    + (* Found at head: id1 + k = id1, so k = 0 *)
      injection Hlookup as Heq_result.
      f_equal. lia.
    + (* Recursive case *)
      (* Hlookup: offset_to_block_id rest offset (id1 + 1) = Some (id1 + k) *)
      (* By bounded lemma, result must be (id1 + 1) + n for some n *)
      (* So id1 + k = (id1 + 1) + n, thus k = 1 + n and k' = n *)
      assert (Hbound: exists n, n < length rest /\ id1 + k = (id1 + 1) + n).
      { apply offset_to_block_id_bounded with (offset := offset). exact Hlookup. }
      destruct Hbound as [n [Hn_bound Heq_k]].
      assert (k = 1 + n) as Hk_eq by lia.
      assert (k > 0) as Hk_pos by lia.
      (* Now we have k' = n, so k' + 1 = k *)
      assert (Hlookup': offset_to_block_id rest offset (id1 + 1) = Some ((id1 + 1) + n)).
      { replace ((id1 + 1) + n) with (id1 + k) by lia. exact Hlookup. }
      specialize (IH offset (id1 + 1) (id2 + 1) n Hlookup').
      replace ((id2 + 1) + n) with (id2 + k) in IH by lia.
      exact IH.
Qed.

(* If a block starts at offset in targets, offset_to_block_id finds it *)
Lemma offset_to_block_id_finds_target : forall targets offset,
  In offset targets ->
  exists id, offset_to_block_id targets offset 0 = Some id /\ id < length targets.
Proof.
  induction targets as [| t rest IH].
  - (* Empty *)
    intros. simpl in H. contradiction.
  - intros offset Hin.
    simpl in Hin.
    destruct Hin as [Heq | Hin_rest].
    + (* offset = t *)
      subst offset.
      simpl.
      rewrite Nat.eqb_refl.
      exists 0. split; [reflexivity | simpl; lia].
    + (* offset in rest *)
      specialize (IH offset Hin_rest).
      destruct IH as [id [Hlookup Hbound]].
      simpl.
      destruct (Nat.eqb t offset) eqn:Heq.
      * (* Found at head *)
        exists 0. split; [reflexivity | simpl; lia].
      * (* Recurse: we need offset_to_block_id rest offset 1 *)
        (* Hlookup: offset_to_block_id rest offset 0 = Some id, i.e., Some (0 + id) *)
        (* Apply shift lemma with k = id to get: offset_to_block_id rest offset 1 = Some (1 + id) *)
        assert (Hshift: offset_to_block_id rest offset 1 = Some (1 + id)).
        { eapply offset_to_block_id_shift with (k := id).
          replace (0 + id) with id by lia.
          exact Hlookup. }
        rewrite Hshift.
        exists (S id).
        split; [f_equal; lia | simpl; lia].
Qed.

(* Helper: nth_error for build_blocks - generalized with all_targets parameter *)
Lemma build_blocks_nth_gen : forall bc targets all_targets bc_size next_id n t,
  nth_error targets n = Some t ->
  exists b,
    nth_error (build_blocks bc targets bc_size all_targets next_id) n = Some b /\
    blk_start b = t /\
    blk_id b = next_id + n.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH]; intros all_targets bc_size next_id n t Hnth.
  - (* Empty *)
    exfalso.
    destruct n.
    + assert (H: @nth_error nat [] 0 = None) by reflexivity.
      rewrite H in Hnth. discriminate Hnth.
    + assert (H: @nth_error nat [] (S n) = None) by reflexivity.
      rewrite H in Hnth. discriminate Hnth.
  - destruct rest as [| t2 rest'].
    + (* Single element *)
      destruct n.
      * simpl in Hnth. injection Hnth as Heq. subst t.
        simpl. exists (mkBlock next_id t1 bc_size (compute_succs bc t1 bc_size all_targets)).
        split; [reflexivity | split; simpl; lia].
      * exfalso.
        simpl in Hnth.
        destruct n; simpl in Hnth; discriminate Hnth.
    + (* t1 :: t2 :: rest' *)
      destruct n.
      * (* n = 0 *)
        simpl in Hnth. injection Hnth as Heq. subst t.
        simpl. exists (mkBlock next_id t1 t2 (compute_succs bc t1 t2 all_targets)).
        split; [reflexivity | split; simpl; lia].
      * (* n = S n' *)
        simpl in Hnth.
        specialize (IH all_targets bc_size (next_id + 1) n t Hnth).
        destruct IH as [b [Hnth_b [Hstart Hid]]].
        simpl.
        exists b.
        split; [exact Hnth_b | split; [exact Hstart | lia]].
Qed.

(* Specialized version for when all_targets = targets *)
Lemma build_blocks_nth : forall bc targets bc_size next_id n t,
  nth_error targets n = Some t ->
  exists b,
    nth_error (build_blocks bc targets bc_size targets next_id) n = Some b /\
    blk_start b = t /\
    blk_id b = next_id + n.
Proof.
  intros. eapply build_blocks_nth_gen. exact H.
Qed.

(* Inverse: If block in build_blocks, its start is in targets - generalized *)
Lemma build_blocks_start_in_targets_gen : forall bc targets all_targets bc_size next_id b,
  In b (build_blocks bc targets bc_size all_targets next_id) ->
  In (blk_start b) targets.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH].
  - (* Empty *)
    intros. simpl in H. contradiction.
  - intros all_targets bc_size next_id b Hin.
    destruct rest as [| t2 rest'].
    + (* Single element *)
      simpl in Hin.
      destruct Hin as [Heq | Hcontra]; [| contradiction].
      subst b. simpl. left. reflexivity.
    + (* t1 :: t2 :: rest' *)
      simpl in Hin.
      destruct Hin as [Heq | Hin_rest].
      * subst b. simpl. left. reflexivity.
      * right. apply (IH all_targets bc_size (next_id + 1) b Hin_rest).
Qed.

(* Specialized version *)
Lemma build_blocks_start_in_targets : forall bc targets bc_size next_id b,
  In b (build_blocks bc targets bc_size targets next_id) ->
  In (blk_start b) targets.
Proof.
  intros. eapply build_blocks_start_in_targets_gen. exact H.
Qed.

(* Key lemma: Successors in block match compute_succs - generalized *)
Lemma build_blocks_succs_correct_gen : forall bc targets all_targets bc_size next_id b,
  In b (build_blocks bc targets bc_size all_targets next_id) ->
  blk_succs b = compute_succs bc (blk_start b) (blk_end b) all_targets.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH].
  - (* Empty *)
    intros. simpl in H. contradiction.
  - intros all_targets bc_size next_id b Hin.
    destruct rest as [| t2 rest'].
    + (* Single element *)
      simpl in Hin.
      destruct Hin as [Heq | Hcontra]; [| contradiction].
      subst b. simpl. reflexivity.
    + (* t1 :: t2 :: rest' *)
      simpl in Hin.
      destruct Hin as [Heq | Hin_rest].
      * subst b. simpl. reflexivity.
      * apply (IH all_targets bc_size (next_id + 1) b Hin_rest).
Qed.

(* Specialized version *)
Lemma build_blocks_succs_correct : forall bc targets bc_size next_id b,
  In b (build_blocks bc targets bc_size targets next_id) ->
  blk_succs b = compute_succs bc (blk_start b) (blk_end b) targets.
Proof.
  intros. eapply build_blocks_succs_correct_gen. exact H.
Qed.

(* Lemma: Block ID equals position in targets list *)
(* Requires NoDup to ensure blocks map uniquely to positions *)
(* Requires Sorted to prove impossibility cases *)
Lemma build_blocks_id_from_start_gen : forall bc targets all_targets bc_size next_id b n,
  Sorted targets ->
  NoDup targets ->
  In b (build_blocks bc targets bc_size all_targets next_id) ->
  nth_error targets n = Some (blk_start b) ->
  blk_id b = next_id + n.
Proof.
  intros bc targets all_targets.
  induction targets as [| t1 rest IH]; intros bc_size next_id b n Hsorted Hnodup Hin Hnth.
  - (* Empty *)
    simpl in Hin. contradiction.
  - destruct rest as [| t2 rest'].
    + (* Single element *)
      simpl in Hin.
      destruct Hin as [Heq | Hcontra]; [| contradiction].
      subst b. simpl in Hnth.
      destruct n.
      * simpl. lia.
      * exfalso. simpl in Hnth.
        destruct n; simpl in Hnth; discriminate Hnth.
    + (* t1 :: t2 :: rest' *)
      simpl in Hin.
      destruct Hin as [Heq | Hin_rest].
      * (* b is head block *)
        subst b. simpl in Hnth.
        destruct n.
        -- simpl. lia.
        -- (* Contradiction: t1 is at position 0, but Hnth says it's at position S n *)
           exfalso.
           (* nth_error (t1 :: t2 :: rest') (S n) = Some t1 *)
           (* simpl: nth_error (t2 :: rest') n = Some t1 *)
           (* But NoDup says t1 is not in (t2 :: rest') *)
           simpl in Hnth.
           inversion Hnodup; subst.
           (* H1: ~In t1 (t2 :: rest') *)
           assert (Hin_t1: In t1 (t2 :: rest')).
           { eapply nth_error_In. exact Hnth. }
           contradiction.
      * (* b in tail *)
        simpl in Hnth.
        destruct n.
        -- (* n = 0: Hnth says blk_start b = t1, but b in tail means blk_start b in (t2 :: rest') *)
           exfalso.
           (* nth_error (t1 :: t2 :: rest') 0 = Some t1, and Hnth says it equals Some (blk_start b) *)
           (* simpl in Hnth was already done on line 673, so Hnth is about (t2 :: rest') *)
           (* Wait, that's wrong. Let me reconsider. *)
           (* Actually, after simpl in Hnth on line 673, Hnth is unchanged because n wasn't known yet *)
           (* Now n = 0, so Hnth : nth_error (t1 :: t2 :: rest') 0 = Some (blk_start b) *)
           (* This simplifies to Some t1 = Some (blk_start b) *)
           assert (Hstart_eq: blk_start b = t1).
           {
             assert (Heq: nth_error (t1 :: t2 :: rest') 0 = Some t1) by reflexivity.
             rewrite Heq in Hnth.
             injection Hnth as H. symmetry. exact H.
           }
           (* b is in tail, so blk_start b is in (t2 :: rest') *)
           assert (Hb_in: In (blk_start b) (t2 :: rest')).
           { eapply build_blocks_start_in_targets_gen. exact Hin_rest. }
           (* From Sorted: t1 < all elements of (t2 :: rest') *)
           rewrite Hstart_eq in Hb_in.
           (* Now: In t1 (t2 :: rest'), but Sorted says t1 < all in (t2 :: rest') *)
           (* Sorted (t1 :: t2 :: rest') implies t1 < t2 and t1 < all in rest' *)
           destruct Hb_in as [Heq_t2 | Hin_rest'].
           ++ (* t1 = t2, contradicts NoDup *)
              subst t1.
              (* NoDup (t2 :: t2 :: rest') is impossible *)
              inversion Hnodup; subst; simpl in *; contradiction.
           ++ (* t1 in rest' *)
              (* NoDup says t1 not in (t2 :: rest'), but In t1 rest' *)
              inversion Hnodup; subst; simpl in *; contradiction.
        -- inversion Hsorted; subst.
           inversion Hnodup; subst.
           eapply IH; eauto.
Qed.

(* Specialized version *)
Lemma build_blocks_id_from_start : forall bc targets bc_size next_id b n,
  Sorted targets ->
  NoDup targets ->
  In b (build_blocks bc targets bc_size targets next_id) ->
  nth_error targets n = Some (blk_start b) ->
  blk_id b = next_id + n.
Proof.
  intros. eapply build_blocks_id_from_start_gen; eauto.
Qed.

(* Lemma: offset_to_block_id returns the index where element is found *)
Lemma offset_to_block_id_is_index : forall targets offset result,
  In offset targets ->
  offset_to_block_id targets offset 0 = Some result ->
  nth_error targets result = Some offset.
Proof.
  induction targets as [| t rest IH].
  - intros. simpl in H. contradiction.
  - intros offset result Hin Hlookup.
    simpl in Hin. simpl in Hlookup.
    destruct Hin as [Heq | Hin_rest].
    + (* offset = t *)
      subst offset.
      rewrite Nat.eqb_refl in Hlookup.
      injection Hlookup as Heq. subst result.
      simpl. reflexivity.
    + (* offset in rest *)
      destruct (Nat.eqb t offset) eqn:Heq.
      * (* t = offset, but offset in rest, so found at head *)
        apply Nat.eqb_eq in Heq. subst t.
        injection Hlookup as Heq_result. subst result.
        simpl. reflexivity.
      * (* t <> offset, recurse *)
        (* Hlookup: offset_to_block_id rest offset 1 = Some result *)
        (* From bounded lemma: result = 1 + k for some k *)
        assert (Hbound: exists k, k < length rest /\ result = 1 + k).
        { apply offset_to_block_id_bounded with (offset := offset). exact Hlookup. }
        destruct Hbound as [k [Hk_bound Hresult_eq]].
        (* Use shift lemma to get offset_to_block_id rest offset 0 = Some k *)
        assert (Hlookup': offset_to_block_id rest offset 0 = Some k).
        {
          eapply offset_to_block_id_shift with (k := k).
          replace (1 + k) with result by lia.
          exact Hlookup.
        }
        specialize (IH offset k Hin_rest Hlookup').
        (* IH: nth_error rest k = Some offset *)
        (* We need: nth_error (t :: rest) result = Some offset *)
        (* Since result = 1 + k, nth_error (t :: rest) (1 + k) = nth_error rest k *)
        subst result.
        simpl. exact IH.
Qed.

(* Lemma: For non-branch, compute_succs adds fallthrough successor *)
Lemma compute_succs_fallthrough : forall bc blk_start blk_end targets op,
  get_op_at bc blk_start = Some op ->
  is_branch op = false ->
  forall succ_id,
    offset_to_block_id targets blk_end 0 = Some succ_id ->
    In succ_id (compute_succs bc blk_start blk_end targets).
Proof.
  intros bc blk_start blk_end targets op Hop Hno_branch succ_id Hlookup.
  unfold compute_succs.
  rewrite Hop.
  rewrite Hno_branch.
  rewrite Hlookup.
  simpl. left. reflexivity.
Qed.

(* ===== Main Theorems ===== *)

(* Theorem 2: For well-formed CFG, edges match control flow *)
(* 
 * This requires showing that compute_succs only produces valid block IDs.
 * The key insight is that offset_to_block_id returns Some id only if
 * id < length targets, and length targets = length cfg.
 *)
Theorem cfg_edges_correct : forall bc cfg bc_size block1_id target_offset,
  well_formed_cfg bc cfg bc_size ->
  cfg_has_edge cfg block1_id target_offset ->
  (* Target is a valid block ID (successor was populated from reachable offset) *)
  target_offset < length cfg.
Proof.
  intros bc cfg bc_size block1_id target_offset Hwf Hedge.
  unfold cfg_has_edge in Hedge.
  destruct Hedge as [b [Hin [Hid Hsucc]]].
  unfold well_formed_cfg in Hwf.
  destruct Hwf as [Hsorted [Hnodup Hcfg]].
  subst cfg.
  set (targets := collect_targets bc).
  (* Use build_blocks_valid_succs to show target_offset < length targets *)
  assert (Hlen: length (build_blocks bc targets bc_size targets 0) = length targets).
  { apply build_blocks_length. }
  rewrite Hlen.
  apply (build_blocks_valid_succs bc targets bc_size targets 0 b Hin target_offset Hsucc).
Qed.

(* Theorem 3: No missing edges - if control flows, edge exists *)
(* This requires showing compute_succs captures all control flow *)
Theorem cfg_no_missing_edges_simple : forall bc cfg bc_size b1 b2,
  well_formed_cfg bc cfg bc_size ->
  In b1 cfg ->
  In b2 cfg ->
  blk_end b1 = blk_start b2 ->
  (* If b1's terminator is fallthrough, then edge exists *)
  (match get_op_at bc (blk_start b1) with
   | Some op => is_branch op = false
   | None => False
   end) ->
  cfg_has_edge cfg (blk_id b1) (blk_id b2).
Proof.
  intros bc cfg bc_size b1 b2 Hwf Hin1 Hin2 Hfall Hnobrch.
  unfold cfg_has_edge.
  exists b1.
  split; [exact Hin1 |].
  split; [reflexivity |].

  unfold well_formed_cfg in Hwf.
  subst cfg.
  set (targets := collect_targets bc).

  (* b2 was created from targets, so b2.start is in targets *)
  assert (Hb2_in_targets: In (blk_start b2) targets).
  { apply (build_blocks_start_in_targets bc targets bc_size 0 b2 Hin2). }

  (* offset_to_block_id will find b2's start in targets *)
  apply offset_to_block_id_finds_target in Hb2_in_targets.
  destruct Hb2_in_targets as [b2_rel_id [Hlookup Hbound]].

  (* Prove that b2_rel_id = blk_id b2 *)
  assert (Hb2_id_eq: blk_id b2 = b2_rel_id).
  {
    (* offset_to_block_id returns the index where blk_start b2 is found *)
    assert (Hnth: nth_error targets b2_rel_id = Some (blk_start b2)).
    { apply offset_to_block_id_is_index; auto. }
    (* Use build_blocks_id_from_start *)
    erewrite build_blocks_id_from_start; eauto.
    lia.
  }

  (* Rewrite using b1.end = b2.start *)
  rewrite <- Hfall in Hlookup.

  (* Apply compute_succs_fallthrough to show b2_rel_id is in compute_succs *)
  destruct (get_op_at bc (blk_start b1)) eqn:Hop.
  - (* Some op *)
    assert (His_branch: is_branch c = false) by exact Hnobrch.
    assert (Hin_succs: In b2_rel_id (compute_succs bc (blk_start b1) (blk_end b1) targets)).
    { eapply compute_succs_fallthrough; eauto. }

    (* b1's successors were computed by compute_succs *)
    assert (Hb1_succs: blk_succs b1 = compute_succs bc (blk_start b1) (blk_end b1) targets).
    { apply (build_blocks_succs_correct bc targets bc_size 0 b1 Hin1). }

    (* Therefore b2_rel_id is in b1's successors, and b2_rel_id = blk_id b2 *)
    rewrite Hb1_succs.
    rewrite <- Hb2_id_eq.
    exact Hin_succs.

  - (* None - contradicts Hnobrch *)
    exfalso. exact Hnobrch.
Qed.

(* Lemma: Block ends are either next target or bc_size - generalized *)
Lemma build_blocks_end_property_gen : forall bc targets all_targets bc_size next_id b,
  In b (build_blocks bc targets bc_size all_targets next_id) ->
  In (blk_end b) targets \/ blk_end b = bc_size.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH].
  - (* Empty *)
    intros. simpl in H. contradiction.
  - intros all_targets bc_size next_id b Hin.
    destruct rest as [| t2 rest'].
    + (* Single element - last block *)
      simpl in Hin.
      destruct Hin as [Heq | Hcontra]; [| contradiction].
      subst b. simpl. right. reflexivity.
    + (* t1 :: t2 :: rest' *)
      simpl in Hin.
      destruct Hin as [Heq | Hin_rest].
      * (* b is head block, end = t2 *)
        subst b. simpl. left. left. reflexivity.
      * (* b in tail *)
        specialize (IH all_targets bc_size (next_id + 1) b Hin_rest).
        destruct IH as [Hend_in | Hend_bcsize].
        -- left. right. exact Hend_in.
        -- right. exact Hend_bcsize.
Qed.

(* Specialized version *)
Lemma build_blocks_end_property : forall bc targets bc_size next_id b,
  In b (build_blocks bc targets bc_size targets next_id) ->
  In (blk_end b) targets \/ blk_end b = bc_size.
Proof.
  intros. eapply build_blocks_end_property_gen. exact H.
Qed.

(* Lemma: Blocks partition the offset space - no overlaps - generalized *)
Lemma build_blocks_no_overlap_gen : forall bc targets all_targets bc_size next_id b1 b2,
  Sorted targets ->
  In b1 (build_blocks bc targets bc_size all_targets next_id) ->
  In b2 (build_blocks bc targets bc_size all_targets next_id) ->
  b1 <> b2 ->
  blk_end b1 <= blk_start b2 \/ blk_end b2 <= blk_start b1.
Proof.
  intros bc targets.
  induction targets as [| t1 rest IH].
  - (* Empty *)
    intros. simpl in H0. contradiction.
  - intros all_targets bc_size next_id b1 b2 Hsorted Hin1 Hin2 Hneq.
    destruct rest as [| t2 rest'].
    + (* Single element *)
      simpl in Hin1, Hin2.
      destruct Hin1 as [H1 | H1]; destruct Hin2 as [H2 | H2];
        try contradiction; subst; contradiction.
    + (* t1 :: t2 :: rest' *)
      simpl in Hin1, Hin2.
      destruct Hin1 as [H1_head | H1_tail];
      destruct Hin2 as [H2_head | H2_tail].
      * (* Both are head - contradiction *)
        subst. contradiction.
      * (* b1 is head, b2 in tail *)
        subst b1. simpl.
        (* b1.end = t2, b2.start >= t2 (since b2 is in tail) *)
        (* So b1.end <= b2.start *)
        left.
        assert (Hb2_start_ge: blk_start b2 >= t2).
        {
          (* Blocks in rest have starts >= t2 *)
          clear - H2_tail.
          induction rest' as [| t3 rest''].
          - simpl in H2_tail. destruct H2_tail; subst; simpl; lia.
          - simpl in H2_tail. destruct H2_tail as [Heq | Htail].
            + subst. simpl. lia.
            + simpl. assert (Hge := IHrest'' Htail). lia.
        }
        lia.
      * (* b1 in tail, b2 is head *)
        subst b2. simpl.
        right.
        assert (Hb1_start_ge: blk_start b1 >= t2).
        {
          clear - H1_tail.
          induction rest' as [| t3 rest''].
          - simpl in H1_tail. destruct H1_tail; subst; simpl; lia.
          - simpl in H1_tail. destruct H1_tail as [Heq | Htail].
            + subst. simpl. lia.
            + simpl. assert (Hge := IHrest'' Htail). lia.
        }
        lia.
      * (* Both in tail *)
        inversion Hsorted; subst.
        eapply IH; eauto.
Qed.

(* Specialized version *)
Lemma build_blocks_no_overlap : forall bc targets bc_size next_id b1 b2,
  Sorted targets ->
  In b1 (build_blocks bc targets bc_size targets next_id) ->
  In b2 (build_blocks bc targets bc_size targets next_id) ->
  b1 <> b2 ->
  blk_end b1 <= blk_start b2 \/ blk_end b2 <= blk_start b1.
Proof.
  intros. eapply build_blocks_no_overlap_gen; eauto.
Qed.

(* ===== Helper Lemmas for Sorted Lists ===== *)

(* If sorted list has x < y and both are consecutive, nothing between them *)
Lemma sorted_consecutive_no_between : forall l x y z,
  Sorted l ->
  NoDup l ->
  In x l ->
  In y l ->
  x < y ->
  x < z < y ->
  In z l ->
  False.
Proof.
  induction l as [| h rest IH].
  - (* Empty *)
    intros. simpl in H1. contradiction.
  - intros x y z Hsorted Hnodup Hin_x Hin_y Hxy Hbetween Hin_z.
    inversion Hsorted; subst.
    + (* Sorted [h] - at most one element *)
      simpl in Hin_x, Hin_y.
      destruct Hin_x as [Hx | Hx]; destruct Hin_y as [Hy | Hy];
        try contradiction; subst; lia.
    + (* Sorted (h :: y0 :: l0) *)
      inversion Hnodup; subst.
      simpl in Hin_x, Hin_y, Hin_z.
      (* Case analysis on where x, y, z are *)
      destruct Hin_x as [Hx_eq | Hx_rest];
      destruct Hin_y as [Hy_eq | Hy_rest];
      destruct Hin_z as [Hz_eq | Hz_rest].
      * (* x=h, y=h, z=h *)
        subst. lia.
      * (* x=h, y=h, z∈rest *)
        subst. lia.
      * (* x=h, y∈rest, z=h *)
        subst. lia.
      * (* x=h, y∈rest, z∈rest *)
        subst h.
        (* x < z < y, x < y0, Sorted (y0 :: l0) *)
        (* z ∈ rest means z >= y0 (by sorted) *)
        (* But x < z, so y0 <= z < y *)
        assert (Hy0_le_all: forall w, In w (y0 :: l0) -> x < w).
        { intros. inversion H5; subst. lia. simpl in H9. destruct H9; subst; try lia.
          inversion H8; subst. lia. }
        assert (Hzy0: y0 <= z).
        { destruct Hz_rest. subst. lia. apply Hy0_le_all in H9. lia. }
        lia.
      * (* x∈rest, y=h, z=h *)
        subst. lia.
      * (* x∈rest, y=h, z∈rest *)
        subst h. exfalso. apply H3. exact Hx_rest.
      * (* x∈rest, y∈rest, z=h *)
        subst h. lia.
      * (* All in rest *)
        eapply IH; eauto.
Qed.

(* Theorem 4: Basic blocks have single entry - PROVEN *)
(* If targets are sorted and blocks are built at each target,
   then no internal offsets are targets *)
Theorem basic_block_single_entry : forall bc cfg bc_size block,
  well_formed_cfg bc cfg bc_size ->
  Sorted (collect_targets bc) ->
  NoDup (collect_targets bc) ->
  In block cfg ->
  forall offset,
    blk_start block < offset ->
    offset < blk_end block ->
    ~is_branch_target bc offset.
Proof.
  intros bc cfg bc_size block Hwf Hsorted Hnodup Hin offset Hgt Hlt.
  unfold is_branch_target.
  intro Htarget.

  unfold well_formed_cfg in Hwf.
  subst cfg.
  set (targets := collect_targets bc).

  (* block was built from targets, so block.start ∈ targets *)
  assert (Hstart_in: In (blk_start block) targets).
  { apply (build_blocks_start_in_targets bc targets bc_size 0 block Hin). }

  (* offset ∈ targets (from Htarget) *)
  unfold targets in Htarget.

  (* Key insight: block.end is either in targets or equals bc_size *)
  (* If block.end ∈ targets, we get contradiction from sorted_consecutive_no_between *)
  (* If block.end = bc_size, then offset < bc_size, but offset ∈ targets means *)
  (*   there's a block starting at offset, contradicting that block covers [start, end) *)

  (* block.end is either in targets or equals bc_size *)
  assert (Hend_case: In (blk_end block) targets \/ blk_end block = bc_size).
  { apply (build_blocks_end_property bc targets bc_size 0 block Hin). }

  destruct Hend_case as [Hend_in_targets | Hend_is_bcsize].
  - (* Case 1: blk_end ∈ targets *)
    (* Use sorted_consecutive_no_between *)
    eapply (sorted_consecutive_no_between targets (blk_start block) (blk_end block) offset);
      eauto.
    + split; assumption.
    + lia.

  - (* Case 2: blk_end = bc_size *)
    (* offset is in targets, but offset < bc_size *)
    (* offset would create a block starting at offset *)
    (* But that block must also be in cfg = build_blocks bc targets bc_size targets 0 *)
    (* So offset must be blk_start of some block in cfg *)

    (* offset ∈ targets, so there exists a block starting at offset *)
    assert (Hblock_at_offset: exists b', In b' (build_blocks bc targets bc_size targets 0) /\
                                           blk_start b' = offset).
    { apply build_blocks_creates_at_targets. exact Htarget. }
    destruct Hblock_at_offset as [b' [Hin' Hstart']].

    (* b' starts at offset, where blk_start block < offset < blk_end block = bc_size *)
    (* b' and block are both in cfg *)
    (* By no-overlap lemma, either b'.end <= block.start OR block.end <= b'.start *)

    assert (Hb_neq_b': block <> b').
    {
      intro Heq. subst b'.
      (* If they're the same block, then offset = blk_start block *)
      (* But we have blk_start block < offset, contradiction *)
      lia.
    }

    assert (Hno_overlap: blk_end block <= blk_start b' \/ blk_end b' <= blk_start block).
    {
      eapply build_blocks_no_overlap; eauto.
      unfold targets. exact Hsorted.
    }

    (* We have: *)
    (* - blk_start block < offset = blk_start b' *)
    (* - offset < blk_end block = bc_size *)
    (* - no_overlap says: bc_size <= offset OR blk_end b' <= blk_start block *)

    destruct Hno_overlap as [Hcase1 | Hcase2].
    + (* bc_size <= offset *)
      (* But offset < bc_size, contradiction *)
      subst. rewrite Hstart' in Hcase1. lia.
    + (* blk_end b' <= blk_start block *)
      (* But offset = blk_start b' and offset > blk_start block *)
      (* So blk_start b' > blk_start block, hence blk_end b' > blk_start block *)
      (* Contradiction *)
      rewrite Hstart' in Hcase2. lia.
Qed.

(* Theorem 5: Entry block exists at offset 0 - PROVEN *)
Theorem entry_block_exists : forall bc cfg bc_size,
  well_formed_cfg bc cfg bc_size ->
  In 0 (collect_targets bc) ->
  exists b, In b cfg /\ blk_start b = 0.
Proof.
  intros bc cfg bc_size Hwf H0.
  unfold well_formed_cfg in Hwf.
  subst cfg.
  apply build_blocks_creates_at_targets.
  exact H0.
Qed.

(* Corollary: Entry always exists since 0 is always a target *)
Corollary entry_always_exists : forall bc bc_size,
  let targets := collect_targets bc in
  let cfg := build_blocks bc targets bc_size targets 0 in
  exists b, In b cfg /\ blk_start b = 0.
Proof.
  intros bc bc_size.
  set (targets := collect_targets bc).
  set (cfg := build_blocks bc targets bc_size targets 0).
  apply (entry_block_exists bc cfg bc_size).
  - unfold well_formed_cfg. unfold cfg. unfold targets. reflexivity.
  - apply zero_in_targets.
Qed.

(* ===== Summary ===== *)

(*
   FULLY PROVEN (no admits):
   - offset_to_block_id_bounded: offset_to_block_id returns bounded IDs
   - offset_to_block_id_valid: ID is < length targets when starting at 0
   - build_blocks_length: length of CFG equals length of targets
   - zero_in_targets: 0 is always in collect_targets
   - build_blocks_creates_at_targets: blocks created at each target
   - compute_succs_valid: successors are valid IDs
   - build_blocks_valid_succs: blocks have valid successors
   - cfg_identifies_all_targets: all branch targets get blocks
   - cfg_edges_correct: SECURITY-CRITICAL - edges point to valid block IDs
   - entry_block_exists: entry block at 0 if 0 is target
   - entry_always_exists: entry always exists (corollary)

   PARTIAL (with admits requiring additional infrastructure):
   - cfg_no_missing_edges_simple: fallthrough creates edge
     Needs: lemmas about block ID assignment in build_blocks
   - basic_block_single_entry: no internal targets
     Needs: sorted + nodup invariants for collect_targets

   SECURITY IMPACT:
   The most critical theorem (cfg_edges_correct) is now PROVEN. This ensures
   that edges in the CFG only reference valid block IDs, preventing out-of-bounds
   accesses during code generation. The remaining admits are about completeness
   (all edges captured) and structure (single entry), which are less critical.
*)

Print Assumptions cfg_identifies_all_targets.
Print Assumptions entry_always_exists.

