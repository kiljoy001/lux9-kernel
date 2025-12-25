(* domtree_spec.v - Coq Specification for CFG and Dominator Tree
 *
 * This specification defines the properties that a correct dominator tree
 * construction must satisfy. It will be used to derive ACSL annotations
 * for the C implementation in cil_domtree.c.
 *)

Require Import List.
Require Import Arith.
Require Import Lia.
Import ListNotations.

(* ===== Basic Types ===== *)

Definition BlockId := nat.
Definition Offset := nat.

(* ===== Control Flow Graph ===== *)

(* Basic block with start/end offsets and successors *)
Record BasicBlock : Type := mkBasicBlock {
  bb_id : BlockId;
  bb_start : Offset;
  bb_end : Offset;
  bb_succs : list BlockId;
  bb_preds : list BlockId
}.

(* CFG is a list of basic blocks *)
Definition CFG := list BasicBlock.

(* Accessor functions *)
Definition get_block (cfg : CFG) (id : BlockId) : option BasicBlock :=
  nth_error cfg id.

Definition successors (cfg : CFG) (id : BlockId) : list BlockId :=
  match get_block cfg id with
  | Some b => bb_succs b
  | None => []
  end.

Definition predecessors (cfg : CFG) (id : BlockId) : list BlockId :=
  match get_block cfg id with
  | Some b => bb_preds b
  | None => []
  end.

(* ===== Edge Relations ===== *)

(* There is an edge from src to dst *)
Definition has_edge (cfg : CFG) (src dst : BlockId) : Prop :=
  In dst (successors cfg src).

(* Path from src to dst *)
Inductive path (cfg : CFG) : BlockId -> BlockId -> Prop :=
| path_refl : forall x, path cfg x x
| path_step : forall x y z, has_edge cfg x y -> path cfg y z -> path cfg x z.

(* ===== Dominance ===== *)

(* Block d dominates block n if every path from entry (0) to n goes through d *)
Definition dominates (cfg : CFG) (d n : BlockId) : Prop :=
  forall p, path cfg 0 n -> path cfg 0 p -> path cfg p n -> path cfg 0 d /\ path cfg d n.

(* Immediate dominator: d is the closest dominator to n *)
Definition is_idom (cfg : CFG) (d n : BlockId) : Prop :=
  dominates cfg d n /\
  d <> n /\
  forall d', dominates cfg d' n -> d' <> n -> dominates cfg d' d \/ d' = d.

(* ===== Dominator Tree ===== *)

Record DomTreeNode : Type := mkDomTreeNode {
  dt_block_id : BlockId;
  dt_idom : BlockId;
  dt_children : list BlockId;
  dt_rpo : nat;
  dt_is_loop_header : bool;
  dt_is_merge_node : bool
}.

Definition DomTree := list DomTreeNode.

Definition get_node (tree : DomTree) (id : BlockId) : option DomTreeNode :=
  nth_error tree id.

Definition node_rpo (tree : DomTree) (id : BlockId) : nat :=
  match get_node tree id with
  | Some n => dt_rpo n
  | None => 0
  end.

Definition node_is_loop_header (tree : DomTree) (id : BlockId) : bool :=
  match get_node tree id with
  | Some n => dt_is_loop_header n
  | None => false
  end.

Definition node_is_merge_node (tree : DomTree) (id : BlockId) : bool :=
  match get_node tree id with
  | Some n => dt_is_merge_node n
  | None => false
  end.

(* ===== Reverse Postorder (RPO) ===== *)

(* RPO property: if there's a path from a to b (and a dominates b), then rpo(a) < rpo(b) *)
(* Exception: back edges go from higher RPO to lower RPO *)

Definition is_forward_edge (tree : DomTree) (src dst : BlockId) : bool :=
  Nat.ltb (node_rpo tree src) (node_rpo tree dst).

Definition is_back_edge (tree : DomTree) (src dst : BlockId) : bool :=
  Nat.leb (node_rpo tree dst) (node_rpo tree src).

(* ===== Loop Header Detection ===== *)

(* A block is a loop header if it has a back edge targeting it *)
Definition has_back_edge_to (cfg : CFG) (tree : DomTree) (target : BlockId) : Prop :=
  exists src, has_edge cfg src target /\ is_back_edge tree src target = true.

Definition ValidLoopHeader (cfg : CFG) (tree : DomTree) (id : BlockId) : Prop :=
  node_is_loop_header tree id = true <-> has_back_edge_to cfg tree id.

(* ===== Back Edge Detection Correctness ===== *)

(* Key theorem: For any edge (src, dst), it's a back edge iff rpo(dst) <= rpo(src) *)
Theorem back_edge_characterization : forall cfg tree src dst,
  has_edge cfg src dst ->
  is_back_edge tree src dst = true <-> 
  node_rpo tree dst <= node_rpo tree src.
Proof.
  intros cfg tree src dst Hedge.
  unfold is_back_edge.
  split.
  - intro H. apply Nat.leb_le. exact H.
  - intro H. apply Nat.leb_le. exact H.
Qed.

(* ===== Merge Node Detection ===== *)

Definition forward_pred_count (cfg : CFG) (tree : DomTree) (id : BlockId) : nat :=
  length (filter (fun pred => is_forward_edge tree pred id) (predecessors cfg id)).

Definition is_merge_node (cfg : CFG) (tree : DomTree) (id : BlockId) : bool :=
  Nat.leb 2 (forward_pred_count cfg tree id).

(* Theorem: A block is a merge node iff it has >= 2 forward predecessors *)
Theorem merge_node_characterization : forall cfg tree id,
  is_merge_node cfg tree id = true <-> forward_pred_count cfg tree id >= 2.
Proof.
  intros cfg tree id.
  unfold is_merge_node.
  split.
  - intro H. apply Nat.leb_le. exact H.
  - intro H. apply Nat.leb_le. exact H.
Qed.

(* ===== Validity Predicates ===== *)

Definition ValidMergeNodeProp (cfg : CFG) (tree : DomTree) (id : BlockId) : Prop :=
  is_merge_node cfg tree id = true <-> node_is_merge_node tree id = true.

Definition ValidIdom (cfg : CFG) (tree : DomTree) (id : BlockId) : Prop :=
  match get_node tree id with
  | Some n => is_idom cfg (dt_idom n) id
  | None => True
  end.

Definition ValidDomTree (cfg : CFG) (tree : DomTree) : Prop :=
  (forall id, id < length tree -> ValidLoopHeader cfg tree id) /\
  (forall id, id < length tree -> ValidMergeNodeProp cfg tree id) /\
  (forall id, id < length tree -> ValidIdom cfg tree id).

Theorem loop_header_iff_back_edge : forall cfg tree id,
  ValidDomTree cfg tree ->
  id < length tree ->
  (has_back_edge_to cfg tree id <-> node_is_loop_header tree id = true).
Proof.
  intros cfg tree id Hvalid Hid.
  destruct Hvalid as [Hloop _].
  specialize (Hloop id Hid).
  unfold ValidLoopHeader in Hloop.
  apply iff_sym. exact Hloop.
Qed.

(* ===== CFG Well-Formedness ===== *)

(* Entry block (0) has no predecessors *)
Definition entry_has_no_preds (cfg : CFG) : Prop :=
  predecessors cfg 0 = [].

(* If (src, dst) is an edge, src is listed in predecessors of dst *)
Definition edges_consistent (cfg : CFG) : Prop :=
  forall src dst, has_edge cfg src dst -> In src (predecessors cfg dst).

(* Reverse: if src is in predecessors of dst, then there's an edge from src to dst *)
Definition preds_implies_edge (cfg : CFG) : Prop :=
  forall src dst, In src (predecessors cfg dst) -> has_edge cfg src dst.

(* All block IDs are valid *)
Definition valid_block_ids (cfg : CFG) : Prop :=
  forall id, id < length cfg -> 
    match get_block cfg id with
    | Some b => bb_id b = id
    | None => False
    end.

(* ===== RPO Numbering Properties ===== *)

(* All RPO numbers are unique *)
Definition rpo_unique (tree : DomTree) : Prop :=
  forall i j, i < length tree -> j < length tree -> i <> j ->
    node_rpo tree i <> node_rpo tree j.

(* RPO numbers are in range [0, n-1] *)
Definition rpo_in_range (tree : DomTree) : Prop :=
  forall i, i < length tree -> node_rpo tree i < length tree.

(* Entry block has RPO 0 *)
Definition entry_has_rpo_0 (tree : DomTree) : Prop :=
  node_rpo tree 0 = 0.

(* ===== Main Correctness Theorems ===== *)

(* If loop header detection is correct, back edges are properly identified *)
(* Forward Direction *)
Theorem loop_header_detection_correct_fwd : forall cfg tree,
  preds_implies_edge cfg ->
  ValidDomTree cfg tree ->
  (forall id pred, id < length tree -> In pred (predecessors cfg id) -> 
    is_back_edge tree pred id = true ->
    node_is_loop_header tree id = true).
Proof.
  intros cfg tree Hpreds Hvalid id pred Hid Hpred Hback.
  destruct Hvalid as [Hloop _].
  specialize (Hloop id Hid).
  apply Hloop.
  exists pred. split.
  - apply Hpreds. exact Hpred.
  - exact Hback.
Qed.

(* Inverse Direction *)
Theorem loop_header_detection_correct_inv : forall cfg tree,
  edges_consistent cfg ->
  ValidDomTree cfg tree ->
  (forall id, id < length tree -> node_is_loop_header tree id = true ->
    exists pred, In pred (predecessors cfg id) /\ is_back_edge tree pred id = true).
Proof.
  intros cfg tree Hconsist Hvalid id Hid Hheader.
  destruct Hvalid as [Hloop _].
  specialize (Hloop id Hid).
  (* ValidLoopHeader: node_is_loop_header = true <-> has_back_edge_to *)
  (* First component: node_is_loop_header = true -> has_back_edge_to *)
  destruct Hloop as [H _].
  specialize (H Hheader).
  destruct H as [pred [Hedge Hback]].
  exists pred. split.
  - apply Hconsist. exact Hedge.
  - exact Hback.
Qed.

(* The dominator tree correctly captures dominance *)
Theorem domtree_captures_dominance : forall cfg tree d n,
  n < length tree ->
  d < length tree ->
  ValidDomTree cfg tree ->
  match get_node tree n with
  | Some node => dt_idom node = d -> dominates cfg d n
  | None => True
  end.
Proof.
  intros cfg tree d n Hn Hd Hvalid.
  destruct Hvalid as [_ [_ Hidom]].
  specialize (Hidom n Hn).
  destruct (get_node tree n) eqn:En; auto.
  unfold ValidIdom in Hidom.
  rewrite En in Hidom.
  intro Heq. subst d.
  destruct Hidom as [Hdom _].
  exact Hdom.
Qed.

Print Assumptions back_edge_characterization.
Print Assumptions merge_node_characterization.
