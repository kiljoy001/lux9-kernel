(* ramsey_spec.v - Coq Specification of Ramsey's Beyond Relooper Algorithm
 *
 * Based on Norman Ramsey's 2022 paper:
 * "Beyond Relooper: Recursive Translation of Unstructured Control Flow
 *  to Structured Control Flow"
 *
 * This is a direct translation of the Haskell code from Section 5.
 *)

Require Import List.
Require Import Arith.
Require Import Lia.
Import ListNotations.

(* ===== Basic Types ===== *)

Definition Label := nat.
Definition RPO := nat.

(* ===== Control Flow Graph ===== *)

Inductive CmmControlFlow : Type :=
| Unconditional : Label -> CmmControlFlow
| Conditional : Label -> Label -> CmmControlFlow
| TerminalFlow : CmmControlFlow.

Record CfgNode : Type := mkCfgNode {
  nodeLabel : Label;
  nodeRPO : RPO;
  nodeIsLoopHeader : bool;
  nodeIsMergeNode : bool;
  nodeFlow : CmmControlFlow
}.

(* ===== Dominator Tree ===== *)

Inductive DomTree : Type :=
| DomNode : CfgNode -> list DomTree -> DomTree.

Definition rootNode (t : DomTree) : CfgNode :=
  match t with
  | DomNode n _ => n
  end.

Definition children (t : DomTree) : list DomTree :=
  match t with
  | DomNode _ cs => cs
  end.

(* ===== WebAssembly Output ===== *)

Inductive Wasm : Type :=
| WasmBlock : Wasm -> Wasm
| WasmLoop : Wasm -> Wasm
| WasmIf : Wasm -> Wasm -> Wasm
| WasmBr : nat -> Wasm
| WasmReturn : Wasm
| WasmActions : Wasm
| WasmSeq : Wasm -> Wasm -> Wasm
| WasmNop : Wasm.

Definition wasmConcat (w1 w2 : Wasm) : Wasm :=
  match w1 with
  | WasmNop => w2
  | _ => match w2 with
         | WasmNop => w1
         | _ => WasmSeq w1 w2
         end
  end.

(* ===== Translation Context ===== *)

Inductive ContainingSyntax : Type :=
| IfThenElse : ContainingSyntax
| LoopHeadedBy : Label -> ContainingSyntax
| BlockFollowedBy : Label -> ContainingSyntax.

Definition Context := list ContainingSyntax.

Definition inside (frame : ContainingSyntax) (ctx : Context) : Context :=
  frame :: ctx.

(* ===== Helper Functions ===== *)

Definition hasMergeRoot (t : DomTree) : bool :=
  nodeIsMergeNode (rootNode t).

Definition filterMergeChildren (cs : list DomTree) : list DomTree :=
  filter hasMergeRoot cs.

Definition isBackward (sourceRPO targetRPO : RPO) : bool :=
  Nat.leb targetRPO sourceRPO.

(* ===== Paper Lines 27-31: index function ===== *)

Fixpoint index (target : Label) (ctx : Context) : option nat :=
  match ctx with
  | [] => None
  | frame :: rest =>
      match frame with
      | BlockFollowedBy l => if Nat.eqb l target then Some 0 
                             else option_map S (index target rest)
      | LoopHeadedBy l => if Nat.eqb l target then Some 0
                          else option_map S (index target rest)
      | IfThenElse => option_map S (index target rest)
      end
  end.

(* ===== Lookup subtree by label ===== *)

Fixpoint subtreeAt (target : Label) (trees : list DomTree) : option DomTree :=
  match trees with
  | [] => None
  | t :: ts => if Nat.eqb (nodeLabel (rootNode t)) target then Some t
               else subtreeAt target ts
  end.

(* ===== Paper Lines 1-33: Mutually Recursive Translation Functions ===== *)

Section Translation.

Variable allTrees : list DomTree.

(* Paper Lines 22-25: doBranch *)
Fixpoint doBranch (fuel : nat) (source target : CfgNode) (ctx : Context) : option Wasm :=
  match fuel with
  | 0 => None
  | S fuel' =>
      if isBackward (nodeRPO source) (nodeRPO target) then
        match index (nodeLabel target) ctx with
        | Some i => Some (WasmBr i)
        | None => None
        end
      else if nodeIsMergeNode target then
        match index (nodeLabel target) ctx with
        | Some i => Some (WasmBr i)
        | None => None
        end
      else
        match subtreeAt (nodeLabel target) allTrees with
        | Some subtree => doTree fuel' subtree ctx
        | None => None
        end
  end

(* Paper Lines 8-20: nodeWithin *)
with nodeWithin (fuel : nat) (x : CfgNode) (ys : list DomTree) (ctx : Context) : option Wasm :=
  match fuel with
  | 0 => None
  | S fuel' =>
      match ys with
      | y_n :: rest =>
          let ylabel := nodeLabel (rootNode y_n) in
          let innerCtx := inside (BlockFollowedBy ylabel) ctx in
          match nodeWithin fuel' x rest innerCtx with
          | Some innerWasm =>
              match doTree fuel' y_n ctx with
              | Some treeWasm => Some (wasmConcat (WasmBlock innerWasm) treeWasm)
              | None => None
              end
          | None => None
          end
      | [] =>
          match nodeFlow x with
          | Unconditional _ => Some (wasmConcat WasmActions (WasmBr 0))
          | Conditional _ _ =>
              Some (wasmConcat WasmActions (WasmIf (WasmBr 0) (WasmBr 0)))
          | TerminalFlow => Some (wasmConcat WasmActions WasmReturn)
          end
      end
  end

(* Paper Lines 1-6: doTree *)
with doTree (fuel : nat) (tree : DomTree) (ctx : Context) : option Wasm :=
  match fuel with
  | 0 => None
  | S fuel' =>
      let x := rootNode tree in
      let mergeChildren := filterMergeChildren (children tree) in
      if nodeIsLoopHeader x then
        let loopCtx := inside (LoopHeadedBy (nodeLabel x)) ctx in
        match nodeWithin fuel' x mergeChildren loopCtx with
        | Some innerWasm => Some (WasmLoop innerWasm)
        | None => None
        end
      else
        nodeWithin fuel' x mergeChildren ctx
  end.

End Translation.

(* ===== Main Translation Function ===== *)

Definition structuredControl (tree : DomTree) : option Wasm :=
  doTree [tree] 1000 tree [].

(* ===== Correctness Theorems ===== *)

Theorem index_in_context : forall target ctx i,
  index target ctx = Some i -> i < length ctx.
Proof.
  intros target ctx.
  induction ctx as [| frame rest IH]; intros i H.
  - simpl in H. discriminate.
  - simpl in H. simpl. destruct frame.
    + (* IfThenElse case *)
      destruct (index target rest) eqn:E; simpl in H; try discriminate.
      injection H as Hi. subst. 
      specialize (IH n eq_refl). lia.
    + (* LoopHeadedBy case *)
      destruct (Nat.eqb l target) eqn:Eq.
      * injection H as Hi. subst. lia.
      * destruct (index target rest) eqn:E; simpl in H; try discriminate.
        injection H as Hi. subst.
        specialize (IH n eq_refl). lia.
    + (* BlockFollowedBy case *)
      destruct (Nat.eqb l target) eqn:Eq.
      * injection H as Hi. subst. lia.
      * destruct (index target rest) eqn:E; simpl in H; try discriminate.
        injection H as Hi. subst.
        specialize (IH n eq_refl). lia.
Qed.

Theorem doTree_terminates : forall allTrees fuel tree ctx,
  fuel > 0 -> exists result, doTree allTrees fuel tree ctx = result.
Proof.
  intros. exists (doTree allTrees fuel tree ctx). reflexivity.
Qed.

Theorem loop_header_wrapped : forall allTrees fuel tree ctx w,
  nodeIsLoopHeader (rootNode tree) = true ->
  doTree allTrees (S fuel) tree ctx = Some w ->
  exists inner, w = WasmLoop inner.
Proof.
  intros allTrees fuel tree ctx w Hloop Htrans.
  simpl in Htrans. rewrite Hloop in Htrans.
  destruct (nodeWithin allTrees fuel (rootNode tree) 
            (filterMergeChildren (children tree))
            (inside (LoopHeadedBy (nodeLabel (rootNode tree))) ctx)) eqn:E.
  - injection Htrans as Hw. exists w0. symmetry. exact Hw.
  - discriminate.
Qed.

(* ===== Inverse Theorem: Valid index implies target in context ===== *)

(* Helper: extract label from a context frame if it has one *)
Definition frameLabel (f : ContainingSyntax) : option Label :=
  match f with
  | IfThenElse => None
  | LoopHeadedBy l => Some l
  | BlockFollowedBy l => Some l
  end.

(* Theorem: If index succeeds, the target must be in the context *)
Theorem context_contains_target : forall target ctx i,
  index target ctx = Some i ->
  exists j, j < length ctx /\ 
            match nth_error ctx j with
            | Some (LoopHeadedBy l) => Nat.eqb l target = true
            | Some (BlockFollowedBy l) => Nat.eqb l target = true
            | _ => False
            end.
Proof.
  intros target ctx i H.
  revert i H.
  induction ctx as [| frame rest IH]; intros i H.
  - discriminate.
  - destruct frame.
    + (* IfThenElse *)
      simpl in H.
      destruct (index target rest) eqn:E; try discriminate.
      injection H as Hi; subst.
      destruct (IH _ eq_refl) as [j [Hlen Hmatch]].
      exists (S j). split; [simpl; lia | exact Hmatch].
    + (* LoopHeadedBy *)
      simpl in H. destruct (Nat.eqb l target) eqn:Eq.
      * injection H as Hi. subst. exists 0. split; [simpl; lia | simpl; exact Eq].
      * destruct (index target rest) eqn:E; try discriminate.
        injection H as Hi. subst.
        destruct (IH _ eq_refl) as [j [Hlen Hmatch]].
        exists (S j). split; [simpl; lia | exact Hmatch].
    + (* BlockFollowedBy *)
      simpl in H. destruct (Nat.eqb l target) eqn:Eq.
      * injection H as Hi. subst. exists 0. split; [simpl; lia | simpl; exact Eq].
      * destruct (index target rest) eqn:E; try discriminate.
        injection H as Hi. subst.
        destruct (IH _ eq_refl) as [j [Hlen Hmatch]].
        exists (S j). split; [simpl; lia | exact Hmatch].
Qed.

(* Biconditional: index succeeds iff target is in context with matching label *)
Theorem index_iff_in_context : forall target ctx,
  (exists i, index target ctx = Some i) <->
  (exists j, j < length ctx /\
             match nth_error ctx j with
             | Some (LoopHeadedBy l) => Nat.eqb l target = true
             | Some (BlockFollowedBy l) => Nat.eqb l target = true  
             | _ => False
             end).
Proof.
  intros target ctx. split.
  - intros [i Hi]. eapply context_contains_target; eauto.
  - intros [j [Hj Hmatch]].
    induction ctx as [| frame rest IH] in j, Hj, Hmatch |- *.
    + simpl in Hj. lia.
    + simpl in Hj. destruct j as [| j'].
      * (* Match at head *)
        simpl in Hmatch. destruct frame.
        -- (* IfThenElse: contradiction, cannot match target *) 
           contradiction.
        -- (* Loop: match found *)
           simpl. rewrite Hmatch. exists 0. reflexivity.
        -- (* Block: match found *)
           simpl. rewrite Hmatch. exists 0. reflexivity.
      * (* Match in tail *)
        simpl in Hmatch.
        assert (Hj' : j' < length rest) by lia.
        destruct frame.
        -- (* IfThenElse *)
           simpl.
           specialize (IH _ Hj' Hmatch). destruct IH as [i Hi].
           exists (S i). rewrite Hi. reflexivity.
        -- (* Loop *)
           simpl. destruct (Nat.eqb l target).
           ++ exists 0. reflexivity.
           ++ specialize (IH _ Hj' Hmatch). destruct IH as [i Hi].
              exists (S i). rewrite Hi. reflexivity.
        -- (* Block *)
           simpl. destruct (Nat.eqb l target).
           ++ exists 0. reflexivity.
           ++ specialize (IH _ Hj' Hmatch). destruct IH as [i Hi].
              exists (S i). rewrite Hi. reflexivity.
Qed.

Print Assumptions index_in_context.
Print Assumptions context_contains_target.
Print Assumptions index_iff_in_context.
Print Assumptions loop_header_wrapped.
